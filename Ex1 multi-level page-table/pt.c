//Yehudis Klughaupt
//יהודית קלוגהויפט


#include "os.h"
#include "stdio.h"

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
    uint64_t pt2=pt, vpn2, siv;
    uint64_t *ptr;
	if (ppn==NO_MAPPING)//Need to destroy
    {
        for (int i = 4; i >= 0; i--) {//go down all tables
            vpn2=vpn>>(i*9);
            siv = (vpn2 & (0x1ff))*8;
            if (i==4)
                ptr=(uint64_t *)(phys_to_virt((pt2)+siv));
            else
                ptr=(uint64_t *)(pt2+siv);
            pt2=*ptr;
            if (((*ptr)&0x1)==0)
                return;
        }
        *ptr&=~0x1;//zero the valid bit
    }
	else//need to allocate
    {
        for (int j = 4; j >=0 ; j--) {
            vpn2=vpn>>(j*9);
            siv = (vpn2 & 0x1ff)*8;
            if (j==4)
                ptr=(uint64_t *)(phys_to_virt(pt2<<12)+siv);
            else
                ptr=(uint64_t *)(pt2+siv);
            if (((*ptr)&0x1)==0)
            {
                if(j!=0)
                {
                    *ptr=(uint64_t)phys_to_virt(alloc_page_frame()<<12);
                    *ptr=(*ptr)^0x1;
                }

            }
            pt2=*ptr;
        }
        *ptr=ppn<<12;//put ppn in right place
        *ptr=(*ptr)^0x1;//turn on valid bit
    }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn)
{
    uint64_t *ptr;
	uint64_t pt2=pt, siv, vpn2;
	//uint64_t offset=vpn&0xfff;
    for (int i = 4; i >=0; i--) {//go down trie of tables
        vpn2=vpn>>(i*9);
        siv = (vpn2 & (0x1ff))*8;
        if (i==4)
            ptr=(uint64_t *)(phys_to_virt(pt2<<12)+siv);
        else
            ptr=(uint64_t *)(pt2+siv);
        if (((*ptr)&0x1)==0)//valid bit is off
            return NO_MAPPING;
        pt2=*ptr;
    }
    return (*ptr)>>12;
}
