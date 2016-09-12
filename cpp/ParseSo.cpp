#include<iostream.h>
#include<string.h>
#include<stdio.h>
#include "elf.h"

/**
	�ǳ���Ҫ��һ���꣬���ܼܺ򵥣�
	P:��Ҫ����Ķε�ַ
	ALIGNBYTES:������ֽ���
	���ܣ���Pֵ���䵽ʱALIGNBYTES��������
	�������Ҳ�У�ҳ����亯��
	eg: 0x3e45/0x1000 == >0x4000
	
*/
#define ALIGN(P, ALIGNBYTES)  ( ((unsigned long)P + ALIGNBYTES -1)&~(ALIGNBYTES-1) )

int addSectionFun(char*, char*, unsigned int);

int main()
{
	addSectionFun("D:\libhello-jni.so", ".jiangwei", 0x1000);
	return 0;
}

int addSectionFun(char *lpPath, char *szSecname, unsigned int nNewSecSize)
{
	char name[50];
	FILE *fdr, *fdw;
	char *base = NULL;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *t_phdr, *load1, *load2, *dynamic;
	Elf32_Shdr *s_hdr;
	int flag = 0;
	int i = 0;
	unsigned mapSZ = 0;
	unsigned nLoop = 0;
	unsigned int nAddInitFun = 0;
	unsigned int nNewSecAddr = 0;
	unsigned int nModuleBase = 0;
	memset(name, 0, sizeof(name));
	if(nNewSecSize == 0)
	{
		return 0;
	}
	fdr = fopen(lpPath, "rb");
	strcpy(name, lpPath);
	if(strchr(name, '.'))
	{
		strcpy(strchr(name, '.'), "_new.so");
	}
	else
	{
		strcat(name, "_new");
	}
	fdw = fopen(name, "wb");
	if(fdr == NULL || fdw == NULL)
	{
		printf("Open file failed");
		return 1;
	}
	fseek(fdr, 0, SEEK_END);
	mapSZ = ftell(fdr);//Դ�ļ��ĳ��ȴ�С
	printf("mapSZ:0x%x\n", mapSZ);

	base = (char*)malloc(mapSZ * 2 + nNewSecSize);//2*Դ�ļ���С+�¼ӵ�Section size
	printf("base 0x%x \n", base);

	memset(base, 0, mapSZ * 2 + nNewSecSize);
	fseek(fdr, 0, SEEK_SET);
	fread(base, 1, mapSZ, fdr);//����Դ�ļ����ݵ�base
	if(base == (void*) -1)
	{
		printf("fread fd failed");
		return 2;
	}

	//�ж�Program Header
	ehdr = (Elf32_Ehdr*) base;
	t_phdr = (Elf32_Phdr*)(base + sizeof(Elf32_Ehdr));
	for(i=0;i<ehdr->e_phnum;i++)
	{
		if(t_phdr->p_type == PT_LOAD)
		{
			//�����flagֻ��һ����־λ��ȥ����һ��LOAD��Segment��ֵ
			if(flag == 0)
			{
				load1 = t_phdr;
				flag = 1;
				nModuleBase = load1->p_vaddr;
				printf("load1 = %p, offset = 0x%x \n", load1, load1->p_offset);

			}
			else
			{
				load2 = t_phdr;
				printf("load2 = %p, offset = 0x%x \n", load2, load2->p_offset);
			}
		}
		if(t_phdr->p_type == PT_DYNAMIC)
		{
			dynamic = t_phdr;
			printf("dynamic = %p, offset = 0x%x \n", dynamic, dynamic->p_offset);
		}
		t_phdr ++;
	}

	//section header
	s_hdr = (Elf32_Shdr*)(base + ehdr->e_shoff);
	//��ȡ���¼�section��λ�ã�������ص�,��Ҫ����ҳ��������
	printf("addr:0x%x\n",load2->p_paddr);
	nNewSecAddr = ALIGN(load2->p_paddr + load2->p_memsz - nModuleBase, load2->p_align);
	printf("new section add:%x \n", nNewSecAddr);

	if(load1->p_filesz < ALIGN(load2->p_paddr + load2->p_memsz, load2->p_align) )
	{
		printf("offset:%x\n",(ehdr->e_shoff + sizeof(Elf32_Shdr) * ehdr->e_shnum));
		//ע������Ĵ����ִ��������������ʵ�����ж�section header�ǲ������ļ���ĩβ
		if( (ehdr->e_shoff + sizeof(Elf32_Shdr) * ehdr->e_shnum) != mapSZ)
		{
			if(mapSZ + sizeof(Elf32_Shdr) * (ehdr->e_shnum + 1) > nNewSecAddr)
			{
				printf("�޷���ӽ�\n");
				return 3;
			}
			else
			{
				memcpy(base + mapSZ, base + ehdr->e_shoff, sizeof(Elf32_Shdr) * ehdr->e_shnum);//��Section Header������ԭ���ļ���ĩβ
				ehdr->e_shoff = mapSZ;
				mapSZ += sizeof(Elf32_Shdr) * ehdr->e_shnum;//����Section Header�ĳ���
				s_hdr = (Elf32_Shdr*)(base + ehdr->e_shoff);
				printf("ehdr_offset:%x",ehdr->e_shoff);
			}
		}
	}
	else
	{
		nNewSecAddr = load1->p_filesz;
	}
	printf("������� %d ����\n", (nNewSecAddr - ehdr->e_shoff) / sizeof(Elf32_Shdr) - ehdr->e_shnum - 1);

	int nWriteLen = nNewSecAddr + ALIGN(strlen(szSecname) + 1, 0x10) + nNewSecSize;//���section֮����ļ��ܳ��ȣ�ԭ���ĳ��� + section name + section size
	printf("write len %x\n",nWriteLen);

	char *lpWriteBuf = (char *)malloc(nWriteLen);//nWriteLen :����ļ����ܴ�С
	memset(lpWriteBuf, 0, nWriteLen);
	//ehdr->e_shstrndx��section name��string����section��ͷ�е�ƫ��ֵ,�޸�string�εĴ�С
	s_hdr[ehdr->e_shstrndx].sh_size = nNewSecAddr - s_hdr[ehdr->e_shstrndx].sh_offset + strlen(szSecname) + 1;
	strcpy(lpWriteBuf + nNewSecAddr, szSecname);//���section name
	
	//���´����ǹ���һ��Section Header
	Elf32_Shdr newSecShdr = {0};
	newSecShdr.sh_name = nNewSecAddr - s_hdr[ehdr->e_shstrndx].sh_offset;
	newSecShdr.sh_type = SHT_PROGBITS;
	newSecShdr.sh_flags = SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR;
	nNewSecAddr += ALIGN(strlen(szSecname) + 1, 0x10);
	newSecShdr.sh_size = nNewSecSize;
	newSecShdr.sh_offset = nNewSecAddr;
	newSecShdr.sh_addr = nNewSecAddr + nModuleBase;
	newSecShdr.sh_addralign = 4;

	//�޸�Program Header��Ϣ
	load1->p_filesz = nWriteLen;
	load1->p_memsz = nNewSecAddr + nNewSecSize;
	load1->p_flags = 7;		//�ɶ� ��д ��ִ��

	//�޸�Elf header�е�section��countֵ
	ehdr->e_shnum++;
	memcpy(lpWriteBuf, base, mapSZ);//��base�п���mapSZ���ȵ��ֽڵ�lpWriteBuf
	memcpy(lpWriteBuf + mapSZ, &newSecShdr, sizeof(Elf32_Shdr));//���¼ӵ�Section Header׷�ӵ�lpWriteBufĩβ
	
	//д�ļ�
	fseek(fdw, 0, SEEK_SET);
	fwrite(lpWriteBuf, 1, nWriteLen, fdw);
	fclose(fdw);
	fclose(fdr);
	free(base);
	free(lpWriteBuf);
	return 0;
}