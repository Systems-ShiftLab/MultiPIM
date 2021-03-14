/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
/*
 * Copyright (C) 2020 Chao Yu (yuchaocs@gmail.com)
 */
#ifndef TLB_ENTRY_H_
#define TLB_ENTRY_H_
#include "common/global_const.h"
#include "galloc.h"
/*---------------TLB entry related-------------*/
class BaseTlbEntry: public GlobAlloc
{
	public: 
		//32bit in none-PAE, 52bit in PAE
		Address v_page_no;
		Address p_page_no;
		uint16_t flag;
		uint64_t lru_seq;	//keep track of LRU seq
		BaseTlbEntry( Address vpn ,Address ppn ):v_page_no(vpn),p_page_no(ppn),flag(0),lru_seq(0)
		{}
		BaseTlbEntry()
		{}
		
		virtual void operator = ( BaseTlbEntry &target_tlb) 
		{
			v_page_no = target_tlb.v_page_no;
			p_page_no = target_tlb.p_page_no;
			flag = target_tlb.flag;
		}

		virtual ~BaseTlbEntry(){}

		bool is_page_global() 
		{ return ( flag & TlbFlag::GLOBAL ); }
		bool is_page_cacheable()
		{ return !( flag & TlbFlag::UNCACHEABLE );	}
		void set_page_shared()
		{
			flag |=  TlbFlag::SHARED;
		}
		bool is_page_shared()
		{ return (flag & TlbFlag::SHARED);}
		void set_page_dirty()
		{
			flag |=  TlbFlag::DIRTY;
		}
		bool is_page_dirty()
		{ return (flag & TlbFlag::DIRTY);}

		bool PAT_enabled()
		{ return ( flag & TlbFlag::PAT);	}
	
		void set_page_global( bool select)
		{ 
		   if( select )
				flag |= TlbFlag::GLOBAL;
		   else
				flag &= (~TlbFlag::GLOBAL);}

		void set_page_cacheable(bool select)
		{
			if( select )
				flag &= (~TlbFlag::UNCACHEABLE);
			else
				flag |= TlbFlag::UNCACHEABLE;
		}

		void enable_PAT(bool select)
		{
			if(select)
				flag |= TlbFlag::PAT;
			else
				flag &= (~TlbFlag::PAT);
		}
		bool is_valid()
		{
			return (flag&TlbFlag::VALID);
		}

		void set_valid()
		{
			flag |= TlbFlag::VALID;
		}
		void set_invalid()
		{
			flag = 0;
		}

		virtual void map( Address vpn , Address ppn )
		{
			v_page_no = vpn;
			p_page_no = ppn;
		}

	   virtual Address get_counter()
	   {	return INVALID_PAGE_ADDR;	}

	   virtual void clear_counter(){}
};

//forward declaration
class TlbEntry;
class TlbEntry: public BaseTlbEntry
{
	public:
	TlbEntry( Address vpn,Address ppn):BaseTlbEntry(vpn ,ppn)
	 {}
	TlbEntry():BaseTlbEntry()
	{}
	//update virtual address
	void update_vpn( Address new_vpn)
	{	v_page_no = new_vpn;	}

	void update_ppn( Address new_ppn )
	{	p_page_no = new_ppn;	}

	void remap( Address ppn)
	{}
};
#endif
