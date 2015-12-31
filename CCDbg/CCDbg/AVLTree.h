#pragma once
#include <iostream>
#include "StuCode.h"
using namespace std;



template <typename T>
class node
{
public:
	T data;
	node* p_parent;
	node* p_left;
	node* p_right;
	int bf;//平衡因子 balance factor
	int depth;
public:
	node()
	{}
	node(T newdata)//构造根节点
	{
		data = newdata;
		depth = 1;
		p_left = p_right = NULL;
		p_parent = NULL;
		bf = 0;
	}
	node(node<T>* p_node, T newdata)//构造非根节点
	{
		data = newdata;
		depth = p_node->depth + 1;
		p_parent = p_node;
		p_left = p_right = NULL;
		bf = 0;
	}
	void print_1()//先根输出
	{
		if(this)
		{
			//             cout<<"address : "<<this<<" data :"<<data<< " "<<this->bf<<" ";
			//             if(p_parent) {cout<<"\tparent:"<<p_parent->data;}
			//             if(p_left)   {cout<<"\tleft:  "<<p_left->data;}
			//             if(p_right)  {cout<<"\tright: "<<p_right->data;}
			//             cout<<endl;
			cout<<data<<" "<<this->bf<<endl;
		}
	}
	void print_2()//中根输出
	{
		if(this)
		{
			cout<<data<<" ";
			//             cout<<"myselft:"<<hex<<data;
			//             if(p_left)   {cout<<"\tleft:  "<<p_left->data;}
			//             if(p_parent) {cout<<"\tparent:"<<p_parent->data;}
			//             if(p_right)  {cout<<"\tright: "<<p_right->data;}
		//             cout<<endl;
		}
	}
	void print_3()//后根输出
	{
		if(this)
		{
			cout<<"myselft:"<<data;
			if(p_left)   {cout<<"\tleft:  "<<p_left->data;}
			if(p_right)  {cout<<"\tright: "<<p_right->data;}
			if(p_parent) {cout<<"\tparent:"<<p_parent->data;}
			cout<<endl;
		}
	}
};

template <typename T>
class c_tree
{
private:
	node<T>* p_root;
	int depth;
	int i_count;
public:
	c_tree()
	{
		p_root = NULL;
		depth = 0;
		i_count = 0;
	}
	~c_tree()//释放数据所占内存空间
	{
		//         cout<<"开始先根释放内存！"<<endl;
		release_all_DLR(p_root);
	}
public:
	node<T>* get_root()
	{
		return p_root;
	}

	int get_count()
	{
		return i_count;
	}

	node<T>* insert(node<T>* p_node, T newdata)
	{
		node<T>* p_new = NULL;
		if(NULL == p_node)
		{
			p_new = new node<T>(newdata);
			p_root = p_new;
			depth = 1;
		}
		else
		{
			p_new = new node<T>(p_node, newdata);
			if (p_node->p_left == NULL)
			{
				p_node->p_left = p_new;
			}
			else if(p_node->p_right == NULL)
			{
				p_node->p_right = p_new;
			}
			else
			{
				if (p_new)
				{
					delete p_new;
				}
				p_new = NULL;
				return NULL;
			}
			if(depth < p_new->depth)
				depth = p_new->depth;
		}
		i_count++;
		return p_new;
	}

	node<T>* sort_insert(T newdata)
	{
		node<T>* p_new = NULL;
		if(NULL == this->p_root)
		{
			p_new = new node<T>(newdata);
			p_root = p_new;
			depth = 1;
		}
		else
		{
			int i_flag_LR = 0;//判断是插在左边还是右边的一个标记值
			node<T>* p_find = find_place(p_root, newdata, i_flag_LR);
			if (p_find == NULL)
			{
				return NULL;
			}
			p_new = new node<T>(newdata);
			if (i_flag_LR == 0)
			{
				p_find->p_left = p_new;
			}
			else
			{
				p_find->p_right = p_new;
			}
			p_new->p_parent = p_find;         
		}
		i_count++;
		return p_new;
	}

	node<T>* find_place(node<T>* p_node, T newdata, int &i_flag_LR)
	{
		node<T>* p_find = p_node;
		if (p_node)
		{
			if (p_node->data > newdata)
			{
				if (p_node->p_left)
				{
					p_find = find_place(p_node->p_left, newdata, i_flag_LR);
				}
				else
				{
					i_flag_LR = 0;
				}
			}
			else
			{
				if (p_node->p_right)
				{
					p_find = find_place(p_node->p_right, newdata, i_flag_LR);
				}
				else
				{
					i_flag_LR = 1;
				}
			}
		}
		return p_find;
	}

	//balance
	node<T>* balance_sort_insert(T newdata)
	{
		node<T>* p_new = NULL;
		node<T>* rotate_node = NULL;//旋转点

		if(NULL == this->p_root)
		{
			p_new = new node<T>(newdata);
			p_root = p_new;
			depth = 1;
			p_new->bf = 0;
		}
		else
		{
			int i_flag_LR = 0;//判断是插在左边还是右边的一个标记值
			node<T>* p_find = find_place(p_root, newdata, i_flag_LR);
			if (p_find == NULL)
			{
				return NULL;
			}
			p_new = new node<T>(newdata);
			p_new->p_parent = p_find;
			p_new->bf = 0;

			if (i_flag_LR == 0)//做为左孩子插入
			{
				p_find->p_left = p_new;
			}
			else//做为右孩子插入
			{
				p_find->p_right = p_new;
			}

			//往根结点方向遍历，修改平衡因子，并在平衡因子绝对值大于1时，进行旋转
			//或者在平衡因子不在变化时，停止向根结点方向的遍历
			node<T>* tmp = p_new;
			while(tmp->p_parent)
			{
				if(tmp->p_parent->bf == -1)         //tmp->p_parent->bf == -1
				{
					if (tmp == tmp->p_parent->p_left)
					{
						tmp->p_parent->bf = 0;
						break;
					}
					else
					{
						tmp->p_parent->bf = -2;
						rotate_node = tmp->p_parent;
						break;
					}
				}
				else if(tmp->p_parent->bf == 0)     //tmp->p_parent->bf == 0
				{
					if (tmp == tmp->p_parent->p_left)
					{
						tmp->p_parent->bf = 1;
						tmp = tmp->p_parent;
					}
					else
					{
						tmp->p_parent->bf = -1;
						tmp = tmp->p_parent;
					}
				}
				else                                //tmp->p_parent->bf == 1
				{
					if (tmp == tmp->p_parent->p_left)
					{
						tmp->p_parent->bf = 2;
						rotate_node = tmp->p_parent;
						break;                    	
					}
					else
					{
						tmp->p_parent->bf = 0;
						break;
					}
				}
			}

			if (rotate_node)//找到旋转点，需要旋转
			{
				if (rotate_node->bf == 2)//左子树比右子树高2层
				{
					if (rotate_node->p_left->bf == 1)//LL型
					{
						//cout<<"LL型调整"<<endl;
						rotate_LL(rotate_node);
					}
					else//LR型  rotate_node->p_left->bf == -1
					{
						//cout<<"LR型调整"<<endl;
						rotate_LR(rotate_node);
					}
				} 
				else//rotate_node->bf == -2 //右子树比左子树高2层
				{
					if (rotate_node->p_right->bf == 1)//RL型
					{
						//cout<<"RL型调整"<<endl;
						rotate_RL(rotate_node);
					}
					else//RR型  rotate_node->p_right->bf == -1
					{
						//cout<<"RR型调整"<<endl;
						rotate_RR(rotate_node);
					}
				}
			}
		}
		i_count++;
		return p_new;
	}

	//LR型调整(先左旋（右边重），后右旋（左边重）)，也就是从整体上来说左边重，从局部来说右边重
	//先处理局部，在处理整体，即先左旋，后右旋。
	void rotate_LR(node<T>* balance_node)
	{
		rotate_RR(balance_node->p_left);
		rotate_LL(balance_node);
	}

	void rotate_RL(node<T>* balance_node)
	{
		rotate_LL(balance_node->p_right);
		rotate_RR(balance_node);
	}

	//          8        插入0后，8的平衡因子从1变为2
	//         / \
	//        6   10     插入0后，6的平衡因子从0变为1
	//      /  \   \
	//     2    7   11   插入0后，2的平衡因子从0变为1
	//    / \  /  \      
	//   1  4 6.5 7.5    插入0后，1的平衡因子从0变为1
	//  /
	// (0)

	//           6             调整后，6的平衡因子从1变为0
	//         /   \
	//       2      8          调整后，8的平衡因子从2变为0
	//      / \    /  \ 
	//     1   4  7   10
	//    /      /  \    \
	//   0      6.5 7.5   11 


	//LL型调整（左边重，向右旋转）
	void rotate_LL(node<T>* balance_node)
	{
		if (balance_node == p_root)
		{
			p_root = balance_node->p_left;
		} 
		//  balance_node = 8
		node<T>* balance_node_parent = balance_node->p_parent;//  null
		node<T>* balance_node_left = balance_node->p_left;    //  6,此值一定存在
		node<T>* balance_node_left_right = balance_node->p_left->p_right; //  7此值有可能为空
		//  6提上8下来
		balance_node_left->p_right = balance_node;
		balance_node->p_parent = balance_node_left;
		balance_node_left->p_parent = balance_node_parent;
		if(balance_node_parent)
		{
			(balance_node_parent->p_left == balance_node)?
				(balance_node_parent->p_left = balance_node_left):
			(balance_node_parent->p_right = balance_node_left);
		}
		//  处理7的关系
		balance_node->p_left = balance_node_left_right;
		if(balance_node_left_right)
		{
			balance_node_left_right->p_parent = balance_node;
		}
		//修改旋转点（8）和旋转点左子结点（6）的平衡因子
		if (balance_node->bf == 2)
		{
			if (balance_node_left->bf == 1)
			{
				balance_node->bf = 0;
				balance_node_left->bf = 0;
			} 
			else if( balance_node_left->bf == 2)
			{
				balance_node->bf = -1;
				balance_node_left->bf = 0;            	
			}
			else if( balance_node_left->bf == 0)//删除的时候遇到的情况
			{
				balance_node->bf = 1;
				balance_node_left->bf = -1;            	
			}
		} 
		else if(balance_node->bf == 1) //RL旋转中的右旋转（双旋转中的第一次）
		{
			if (balance_node_left->bf == 1)
			{
				balance_node_left->bf = -1;
				balance_node->bf = -1;
			} 
			else if ( balance_node_left->bf == -1)
			{
				balance_node_left->bf = -2;
				balance_node->bf = 0;
			}
			else if ( balance_node_left->bf == 0)
			{
				balance_node_left->bf = -1;
				balance_node->bf = 0;
			}
		}
	}

	//RR型调整（右边重，向左旋转）
	void rotate_RR(node<T>* balance_node)
	{
		if (balance_node == p_root)
		{
			p_root = balance_node->p_right;
		} 
		node<T>* balance_node_parent = balance_node->p_parent;
		node<T>* balance_node_right = balance_node->p_right;    //  此值一定存在
		node<T>* balance_node_right_left = balance_node->p_right->p_left; //  此值有可能为空
		balance_node_right->p_left = balance_node;
		balance_node->p_parent = balance_node_right;
		balance_node_right->p_parent = balance_node_parent;
		if(balance_node_parent)
		{
			(balance_node_parent->p_left == balance_node)?
				(balance_node_parent->p_left = balance_node_right):
			(balance_node_parent->p_right = balance_node_right);
		}
		//  处理旋转点右子节点的左子结点
		balance_node->p_right = balance_node_right_left;
		if(balance_node_right_left)
		{
			balance_node_right_left->p_parent = balance_node;
		}
		//修改 旋转点 和 旋转点右子结点 的平衡因子
		if (balance_node->bf == -2)  
		{
			if (balance_node_right->bf == -1)    //普通的左旋转
			{
				balance_node->bf = 0;
				balance_node_right->bf = 0;
			} 
			else if(balance_node_right->bf == -2) //双旋转中的左旋
			{
				balance_node->bf = 1;
				balance_node_right->bf = 0;            	
			}
			else if(balance_node_right->bf == 0)//删除的时候遇到的情况
			{
				balance_node->bf = -1;
				balance_node_right->bf = 1;            	
			}

		} 
		else if(balance_node->bf == -1) //LR旋转中的左旋转 
		{
			if (balance_node_right->bf == 1)
			{
				balance_node_right->bf = 2;
				balance_node->bf = 0;
			} 
			else if(balance_node_right->bf == -1)
			{
				balance_node_right->bf = 1;
				balance_node->bf = 1;
			}
			else if(balance_node_right->bf == 0)
			{
				balance_node_right->bf = 1;
				balance_node->bf = 0;
			}
		}
	}

	node<T>* find_data(T data)
	{
		return find(p_root, data);
	}

	node<T>* find(node<T>* p_node, T data)
	{
		if (p_node)
		{
			if(p_node->data == data)
			{
				return p_node;
			}
			else if(data < p_node->data)
			{
				return find(p_node->p_left, data);
			}
			else
			{
				return find(p_node->p_right, data);
			}            
		}
		else
		{
			return NULL;
		}        
	}

	bool balance_delete_node(node<T>* p_node)
	{
		if (p_node)
		{
			node<T>* p_change_node = NULL;
			if (p_node->p_left == NULL && p_node->p_right == NULL)//左右子树为空
			{
				if (p_node == p_root)
				{
					p_root = NULL;
					i_count--;
					return true;
				}
				if (p_node->p_parent)
				{
					if(p_node->p_parent->p_left == p_node)
					{
						p_node->p_parent->p_left = NULL;
						p_node->p_parent->bf = p_node->p_parent->bf - 1;
					}
					else
					{
						p_node->p_parent->p_right = NULL;
						p_node->p_parent->bf = p_node->p_parent->bf + 1;
					}
				}
				p_change_node = p_node->p_parent;
			}
			else if (p_node->p_left == NULL && p_node->p_right != NULL)//左子树为空
			{
				p_node->p_right->p_parent = p_node->p_parent;

				if (p_node == p_root)
				{
					p_root = p_node->p_right;
				}
				//调整p_node的父结点的平衡因子，并向根回溯，看是否需要旋转
				//p_node的位置被p_node->p_right代替。
				if (p_node->p_parent)
				{
					if(p_node->p_parent->p_left == p_node)
					{
						p_node->p_parent->p_left = p_node->p_right;
						p_node->p_parent->bf = p_node->p_parent->bf - 1;
					}
					else
					{
						p_node->p_parent->p_right = p_node->p_right;
						p_node->p_parent->bf = p_node->p_parent->bf + 1;
					}
				} 
				p_change_node = p_node->p_parent;
			} 
			else if(p_node->p_right == NULL && p_node->p_left != NULL)//右子树为空
			{
				p_node->p_left->p_parent = p_node->p_parent;

				if (p_node == p_root)
				{
					p_root = p_node->p_left;
				}
				//调整p_node的父结点的平衡因子，并向根回溯，看是否需要旋转
				//p_node的位置被p_node->p_left代替。
				if (p_node->p_parent)
				{
					if(p_node->p_parent->p_left == p_node)
					{
						p_node->p_parent->p_left = p_node->p_left;
						p_node->p_parent->bf = p_node->p_parent->bf - 1;
					}
					else
					{
						p_node->p_parent->p_right = p_node->p_left;
						p_node->p_parent->bf = p_node->p_parent->bf + 1;
					}
				} 
				p_change_node = p_node->p_parent;
			}
			else//左右子树都不为空
			{
				//用p_node的直接前缀代替p_node
				node<T>* p_tmp = p_node->p_left;//p_node->p_left肯定存在
				while(p_tmp->p_right)
				{
					p_tmp = p_tmp->p_right;
				}

				if (p_node == p_root)
				{
					p_root = p_tmp;
				}

				//特例：p_tmp == p_node->p_left, p_tmp的右子树为空
				if (p_tmp == p_node->p_left)
				{
					//p_tmp代替p_node
					p_tmp->p_right = p_node->p_right;
					p_node->p_right->p_parent = p_tmp;
					p_tmp->p_parent = p_node->p_parent;
					if (p_node->p_parent)
					{
						if (p_node->p_parent->p_left == p_node)
						{
							p_node->p_parent->p_left = p_tmp;
						}
						else
						{
							p_node->p_parent->p_right = p_tmp;
						}
					}
					p_tmp->bf = p_node->bf - 1;
					p_change_node = p_tmp;
				} 
				else//一般情况
				{
					//设置tmp的父结点的右结点指向tmp的左结点
					p_tmp->p_parent->p_right = p_tmp->p_left;
					if (p_tmp->p_left)//设置tmp的左结点的父结点指向tmp的父结点
					{
						p_tmp->p_left->p_parent = p_tmp->p_parent;
					}
					p_tmp->p_parent->bf = p_tmp->p_parent->bf + 1;
					p_change_node = p_tmp->p_parent;

					p_tmp->p_parent = p_node->p_parent;
					p_tmp->bf = p_node->bf;//p_tmp占据p_node的位置后，继承p_node的BF

					if (p_node->p_parent)
					{
						if (p_node->p_parent->p_left == p_node)
						{
							p_node->p_parent->p_left = p_tmp;
							//p_node->p_parent->bf = p_node->p_parent->bf - 1;
						}
						else
						{
							p_node->p_parent->p_right = p_tmp;
							//p_node->p_parent->bf = p_node->p_parent->bf + 1;
						}
					}

					p_tmp->p_left = p_node->p_left;
					p_tmp->p_right = p_node->p_right;

					p_node->p_left->p_parent = p_tmp;
					p_node->p_right->p_parent = p_tmp;
				}
			}

			if (p_change_node)//回溯进行平衡性检查
			{
				balance_adjust(p_change_node);
			}

			delete p_node;
			p_node = NULL;
			i_count--;
			return true;
		} 
		else
		{
			return false;
		}
	}

	void balance_adjust(node<T>* p_node)
	{
		if (p_node)
		{
			bool p_node_is_left_of_parent = true;
			node<T>* p_node_parent = p_node->p_parent;
			node<T>* p_node_copy = p_node;
			//以上两句，之所以要保存p_node及其父结点，是因为在旋转过程当中会破坏他们的数据联系
			//同样，下面的变量p_node_is_left_of_parent记录p_node是否是父结点的左孩子
			//必须提前判断，否则旋转之后他们可能不是父子结点的关系
			if (p_node_parent)
			{
				p_node_is_left_of_parent = (p_node == p_node_parent->p_left);
			}
			if (p_node->bf == 1 || p_node->bf == -1)//树高无变化
			{
				return;        	
			} 
			else if(p_node->bf == 0)//树高降低，会影响父结点的BF
			{
				//调整父结点的BF
				if (p_node_parent)
				{
					(p_node_is_left_of_parent) ?
						(p_node_parent->bf = p_node_parent->bf - 1) :
					(p_node_parent->bf = p_node_parent->bf + 1);
				}
				balance_adjust(p_node_parent);
			}
			else if(p_node->bf == 2)//需要旋转
			{
				if (p_node->p_left->bf == 1)//LL旋转  树高降低，需继续回溯
				{
					rotate_LL(p_node);//注意，旋转之后改变了p_node的父子结点
					//调整父结点的BF
					if (p_node_parent)
					{
						(p_node_is_left_of_parent) ?
							(p_node_parent->bf = p_node_parent->bf - 1) :
						(p_node_parent->bf = p_node_parent->bf + 1);
					}
					balance_adjust(p_node_parent);
				} 
				else if(p_node->p_left->bf == -1)//LR双旋  树高降低，需继续回溯
				{
					rotate_LR(p_node);
					//调整父结点的BF
					if (p_node_parent)
					{
						(p_node_is_left_of_parent) ?
							(p_node_parent->bf = p_node_parent->bf - 1) :
						(p_node_parent->bf = p_node_parent->bf + 1);
					}
					balance_adjust(p_node_parent);
				}
				else if(p_node->p_left->bf == 0)//LL旋转  树高不变，回溯停止
				{
					rotate_LL(p_node);
					return;
				}                
			}
			else if(p_node->bf == -2)//需要旋转
			{
				if (p_node->p_right->bf == -1)//RR旋转  树高降低，需继续回溯
				{
					rotate_RR(p_node);
					//调整父结点的BF
					if (p_node_parent)
					{
						(p_node_is_left_of_parent) ?
							(p_node_parent->bf = p_node_parent->bf - 1) :
						(p_node_parent->bf = p_node_parent->bf + 1);
					}
					balance_adjust(p_node_parent);
				} 
				else if(p_node->p_right->bf == 1)//RL双旋  树高降低，需继续回溯
				{
					rotate_RL(p_node);
					//调整父结点的BF
					if (p_node_parent)
					{
						(p_node_is_left_of_parent) ?
							(p_node_parent->bf = p_node_parent->bf - 1) :
						(p_node_parent->bf = p_node_parent->bf + 1);
					}
					balance_adjust(p_node_parent);
				}
				else if(p_node->p_right->bf == 0)//RR旋转  树高不变，回溯停止
				{
					rotate_RR(p_node);
					return;
				}
			}
		}
		else
		{
			return;
		}
	}


	void print_DLR(node<T>* p_node)//先根输出
	{
		if (!p_node)
		{
			return;
		}
		p_node->print_1();
		if(p_node->p_left)
		{
			print_DLR(p_node->p_left);
		}
		if(p_node->p_right)
		{
			print_DLR(p_node->p_right);
		}
	}

	void print_LDR(node<T>* p_node)//中根输出
	{
		if (!p_node)
		{
			return;
		}
		if(p_node->p_left)
		{
			print_LDR(p_node->p_left);
		}
		p_node->print_1();
		if(p_node->p_right)
		{
			print_LDR(p_node->p_right);
		}
	}

	void write_array(int* p, int* pbf, node<T>* p_node, int i_start, int i_end)//中根输出到数组
	{
		int i_center = (i_start + i_end) / 2;
		if (p_node)
		{
			p[i_center-1] = p_node->data;
			pbf[i_center-1] = p_node->bf;
			write_array(p, pbf, p_node->p_left, i_start, i_center-1);
			write_array(p, pbf, p_node->p_right, i_center+1, i_end);
		}
	}

	void print_LRD(node<T>* p_node)//后根输出
	{
		if (!p_node)
		{
			return;
		}
		if(p_node->p_left)
		{
			print_LRD(p_node->p_left);
		}
		if(p_node->p_right)
		{
			print_LRD(p_node->p_right);
		}
		p_node->print_3();
	}

	void release_all(node<T>* p_node)
	{
		if (p_node == NULL)
		{
			return;
		}

		if (p_node->p_left)
		{
			release_all(p_node->p_left);
		}
		if (p_node->p_right)
		{
			release_all(p_node->p_right);
		}
		if (p_node)
		{
			//cout<<"release "<<p_node->data<<endl;
			delete p_node;
			p_node = NULL;
		}        
	}

	bool delete_node(node<T>* p_node)//没有处理depth
	{
		if (!p_node)
		{
			return false;
		}
		if (!p_node->p_left)
		{
			if(p_node->p_right)
				p_node->p_right->p_parent = p_node->p_parent;
			if (p_node->p_parent)
			{
				(p_node->p_parent->p_right == p_node) ? 
					(p_node->p_parent->p_right = p_node->p_right) :
				(p_node->p_parent->p_left = p_node->p_right);
			}
			if (p_node == this->p_root)
			{
				p_root = p_node->p_right;
			}
		} 
		else if(!p_node->p_right)
		{
			if(p_node->p_right)
				p_node->p_right->p_parent = p_node->p_parent;
			if(p_node->p_parent)
			{
				(p_node->p_parent->p_right == p_node) ? 
					(p_node->p_parent->p_right = p_node->p_right) :
				(p_node->p_parent->p_left = p_node->p_right);
			}
			if (p_node == this->p_root)
			{
				p_root = p_node->p_left;
			}
		}
		else
		{
			node<T>* tmp = p_node->p_left;
			while(tmp->p_right)//tmp为p_node的左子树的最右下角
			{
				tmp = tmp->p_right;
			}
			if (tmp == p_node->p_left)//如果p_node的左子树的最右下角就是左子树根结点本身
			{
				tmp->p_parent = p_node->p_parent;
				if(p_node->p_parent)
				{
					(p_node->p_parent->p_right == p_node) ? 
						(p_node->p_parent->p_right = tmp) :
					(p_node->p_parent->p_left = tmp);
				}
				tmp->p_right = p_node->p_right;
				p_node->p_right->p_parent = tmp;
			}
			else
			{
				tmp->p_parent->p_right = tmp->p_left;//设置tmp的父结点的右结点指向tmp的左结点
				if (tmp->p_left)//设置tmp的左结点的父结点指向tmp的父结点
				{
					tmp->p_left->p_parent = tmp->p_parent;
				}

				tmp->p_parent = p_node->p_parent;
				if(p_node->p_parent)
				{
					(p_node->p_parent->p_right == p_node) ? 
						(p_node->p_parent->p_right = tmp) :
					(p_node->p_parent->p_left = tmp);
				}
				tmp->p_left = p_node->p_left;
				tmp->p_right = p_node->p_right;

				p_node->p_left->p_parent = tmp;
				p_node->p_right->p_parent = tmp;
			}
			if (p_node == this->p_root)
			{
				p_root = tmp;
			}
		}
		delete p_node;
		p_node = NULL;
		this->i_count--;
		return true;
	}

	void release_all_DLR(node<T>* p_node)
	{
		if (!p_node)
		{
			return;
		}
		node<T>* p_tmp_left = p_node->p_left;
		node<T>* p_tmp_right = p_node->p_right;
		if (p_node)
		{
			//             cout<<"release "<<p_node->data<<endl;
			delete p_node;
			p_node = NULL;
		}
		if (p_tmp_left)
		{
			release_all_DLR(p_tmp_left);
		}
		if (p_tmp_right)
		{
			release_all_DLR(p_tmp_right);
		}
	}

};


