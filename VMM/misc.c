#include "misc.h"
#include "vmx.h"


NTSTATUS InitializeSegmentSelector(PSEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase)
{
	PSEGMENT_DESCRIPTOR SegDesc = NULL;

	if (!SegmentSelector)
		return STATUS_INVALID_PARAMETER;

	/*
	[1 : 0]��RPL ��0��3
	[2]��TI��Table Index����Ϊ0ʱ ��GDT���ң�Ϊ1ʱ����LDT����
	[15 : 3]��Descriptor Index���� GDT/LDT �е����
	*/
	if (Selector & 0x4)  //���TIλΪ1��������GDT��
	{
		DbgPrint("InitializeSegmentSelector(): Given selector (0x%X) points to LDT\n", Selector);
		return STATUS_INVALID_PARAMETER;
	}
	//													ȡIndex	 1000 <- 0111
	       
	SegDesc = (PSEGMENT_DESCRIPTOR)((PUCHAR)GdtBase + (Selector & ~0x7));//ָ���ƫ��

	SegmentSelector->sel = Selector;
	SegmentSelector->base = SegDesc->base0 | SegDesc->base1 << 16 | SegDesc->base2 << 24;
	SegmentSelector->limit = SegDesc->limit0 | (SegDesc->limit1attr1 & 0xf) << 16;
	SegmentSelector->attributes.UCHARs = SegDesc->attr0 | (SegDesc->limit1attr1 & 0xf0) << 4;

	if (!(SegDesc->attr0 & LA_STANDARD))  //P ��־λ ָʾ�Ƿ����ڴ��У�����
	{
		// this is a TSS or callgate etc, save the base high part
		ULONG64 tmp = (*(PULONG64)((PUCHAR)SegDesc + 8));
		SegmentSelector->base = (SegmentSelector->base & 0xffffffff) | (tmp << 32);
	}

	if (SegmentSelector->attributes.fields.g)  //�����ȣ�1��4KB��0��1 byte
	{
		// 4096-bit granularity is enabled for this segment, scale the limit
		/*
		��segment descriptor��limit20λ����չ��32λ
		*/
		SegmentSelector->limit = (SegmentSelector->limit << 12) + 0xfff;
	}

	return STATUS_SUCCESS;
}



NTSTATUS FillGuestSelectorData(PVOID GdtBase, ULONG Segreg, USHORT Selector) //ES DS �Ĵ����д�ŵľ���selector
{
	SEGMENT_SELECTOR SegmentSelector = { 0 };
	ULONG uAccessRights;

	InitializeSegmentSelector(&SegmentSelector, Selector, (PUCHAR)GdtBase);
	uAccessRights = ((PUCHAR)& SegmentSelector.attributes)[0] + (((PUCHAR)&
		SegmentSelector.attributes)[1] << 12);     //������

	if (!Selector)
		uAccessRights |= 0x10000;
	//����� �ֶ�ID ����ES��Ϊ��׼��Ȼ����ƫ�ƣ���Ϊ����2
	__vmx_vmwrite(GUEST_ES_SELECTOR + Segreg * 2, Selector);
	__vmx_vmwrite(GUEST_ES_LIMIT + Segreg * 2, SegmentSelector.limit);
	__vmx_vmwrite(GUEST_ES_AR_BYTES + Segreg * 2, uAccessRights);

	// Don't setup for FS/GS - their bases are stored in MSR values
	if ((Segreg == LDTR) || (Segreg == TR))
		__vmx_vmwrite(GUEST_ES_BASE + Segreg * 2, SegmentSelector.base);

	return STATUS_SUCCESS;
}


ULONG AdjustControls(ULONG Ctl, ULONG Msr)
{
	LARGE_INTEGER msrValue;
	msrValue.QuadPart = __readmsr(Msr);

	DbgLog("Adjusting control for msr 0x%x\n", Msr);
	DbgLog("Adjusting controls (low): 0x%08x\n", msrValue.LowPart);
	DbgLog("Adjusting controls (high): 0x%08x\n", msrValue.HighPart);

	Ctl &= msrValue.HighPart;
	Ctl |= msrValue.LowPart;
	return Ctl;
}