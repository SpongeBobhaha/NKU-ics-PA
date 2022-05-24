#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })
#define PTE_ADDR(pte)   ((uint32_t)(pte)&~0xfff)

#define PDX(va)    (((uint32_t)(va)>>22)&0x3ff)
#define PTX(va)    (((uint32_t)(va)>>12)&0x3ff)
#define OFF(va)    ((uint32_t)(va)&0xfff)
uint8_t pmem[PMEM_SIZE];
///////////////////////

paddr_t page_translate(vaddr_t addr,bool iswrite){
	CR0 cr0=(CR0)cpu.CR0;
	if(cr0.paging&&cr0.protect_enable){
		CR3 cr3=(CR3)cpu.CR3;
		PDE* pgdirs =(PDE*)PTE_ADDR(cr3.val);
		PDE pde=(PDE)paddr_read((uint32_t)(pgdirs+PDX(addr)),4);
		Assert(pde.present,"addr=0x%x",addr);

		PTE* ptab=(PTE*)PTE_ADDR(pde.val);
		PTE pte=(PTE)paddr_read((uint32_t)(ptab+PTX(addr)),4);
		Assert(pte.present,"addr=0x%x",addr);

		pde.accessed=1;
		pte.accessed=1;
		if(iswrite){
			pte.dirty=1;
		}

		paddr_t paddr= PTE_ADDR(pte.val)|OFF(addr);
		return paddr;

	}
	return addr;
}

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
	int r=is_mmio(addr);
	//printf("fasfaaaaaaaaaaaaaaaaaaaaaaaaaa");
	if(r!=-1)
		return mmio_read(addr,len,r);
	else
		return pmem_rw(addr,uint32_t)&(~0u>>((4-len)<<3));
  //return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
	int r=is_mmio(addr);
	if(r!=-1)
		return mmio_write(addr,len,data,r);
	else
		memcpy(guest_to_host(addr),&data,len);	
  //memcpy(guest_to_host(addr), &data, len);
}

uint32_t vaddr_read(vaddr_t addr, int len) {
	if(PTE_ADDR(addr)!=PTE_ADDR(addr+len-1)){
		/*printf("error:the data pass two pages:addr=0x%x,len=%d!\n",addr,len);
		assert(0);	*/
		int num1 = 0x1000-OFF(addr);
		int num2 = len-num1;
		paddr_t paddr1=page_translate(addr,false);
		paddr_t paddr2=page_translate(addr+num1,false);

		uint32_t low=paddr_read(paddr1,num1);
		uint32_t high=paddr_read(paddr2,num2);

		uint32_t result=high<<(num1*8)|low;
		return result;	
	}else{
		paddr_t paddr=page_translate(addr,false);
		return paddr_read(paddr, len);
	}
  
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
	  if(PTE_ADDR(addr)!=PTE_ADDR(addr+len-1)){
		/*printf("error:the data pass two pages:addr=0x%x,len=%d!\n",addr,len);*/
		int num1 = 0x1000-OFF(addr);
		int num2 = len-num1;
		paddr_t paddr1=page_translate(addr,true);
		paddr_t paddr2=page_translate(addr+num1,true);

		uint32_t low=data & (~0u >>((4-num1)<<3));
		uint32_t high=data>>((4-num2)<<3);
		
		paddr_write(paddr1,num1,low);
		paddr_write(paddr2,num2,high);
		
		return ;	
		assert(0);		
	}else{
		paddr_t paddr=page_translate(addr,true);
		paddr_write(paddr, len, data);
	}
}

