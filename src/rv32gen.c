//-------------------------------------------------------------------------------------------------
//                                                                           
//  RV32GEN - RISC-V 32bits IM Random Instruction Generator                                             
//  Copyright (C) 2021-2022 HT-LAB                                           
//                                                                                                           
//                                                                           
//-------------------------------------------------------------------------------------------------
//  https://github.com/htminuslab                                                       
//-------------------------------------------------------------------------------------------------
//
//  Revision History: 
//                                                                           
//  Date:          Revision         Author      
//  12-12-21       Created			Hans			Copied bits from gen486 code
//  16-10-22       Updated			Hans			Added memory base address
//-------------------------------------------------------------------------------------------------
#include "rv32gen.h"

#if NDEBUG
	#define dprintf(fmt, ARGS...) 
#else
	#define dprintf(fmt, ARGS...) fprintf(stdout, fmt, ##ARGS); 
#endif

//-------------------------------------------------------------------------------------------------
// Global Variables
//-------------------------------------------------------------------------------------------------				
int  debug=0;            											// debug display
bool quiet=false;		
char march[]="imc";													// Integer, Mult/Div, compressed

enum opc32I {
		LUI,AUIPC,JAL,JALR,BEQ,BNE,BLT,BGE,BLTU,BGEU,LB,LH,LW,LBU,LHU,SB,SH,SW,
		ADDI,SLTI,SLTIU,XORI,ORI,ANDI,SLLI,SRLI,SRAI,ADD,SUB,SLL,SLT,SLTU,XOR,
		SRL,SRA,OR,AND,FENCE,ECALL,EBREAK,
		MUL,MULH,MULHSU,MULHU,DIV,DIVU,REM,REMU,INVALID};
	
const char * const opc32Is[] = {
		"LUI","AUIPC","JAL","JALR","BEQ","BNE","BLT","BGE","BLTU","BGEU","LB","LH","LW","LBU","LHU","SB","SH","SW",
        "ADDI","SLTI","SLTIU","XORI","ORI","ANDI","SLLI","SRLI","SRAI","ADD","SUB","SLL","SLT","SLTU","XOR",
		"SRL","SRA","OR","AND","FENCE","ECALL","EBREAK",
		"MUL","MULH","MULHSU","MULHU","DIV","DIVU","REM","REMU","INVALID"};

const char * const regs[] = {
		"zero","ra","sp","gp","tp","t0","t1","t2","s0","s1","a0","a1","a2","a3","a4","a5",
		"a6","a7","s2","s3","s4","s5","s6","s7","s8","s9","s10","s11","t3","t4","t5","t6"}; 
		
const char * const plus[] = {"+","-"};

//-------------------------------------------------------------------------------------------------
// Main Entry 
//-------------------------------------------------------------------------------------------------
int main(int argc,char **argv)
{
    int 	i=1; 													// Command line argument
	
	char 	asmfilename[80]=""; 									// Output Assemblt file
	FILE 	*fp=NULL;                      						

	bool 	gen_ebreak=false;
	bool 	gen_ecall=false;
	
	enum 	opc32I opc;
	char 	buf[MAX_LINE];
	uint32_t icnt=0,icnt_max=512;									// Default generate 512 instructions, change with -ic <hex> option
	
	int 	linkreg=0;
	int 	offset=0;
	int 	labelcnt=0;
	unsigned int seed;												// Change random function
	uint32_t base_address=0x1000;									// Default memory address, minimum 2048 as we generate 12bits offsets
		
    if (argc < 1) usage_exit();
	
	seed=(unsigned)time(NULL);										// Use time as default
	
	//---------------------------------------------------------------------------------------------
	// Processing Command line argument(s) 
	//---------------------------------------------------------------------------------------------
	while((argc-1)>=i)	{		
		if (strcmp(argv[i],"-q")==0) {								// quiet mode
			quiet=true;												// Do not display headers and stuff
			i++;						
		} else if (strcmp(argv[i],"-ebreak")==0) {					// Disable generating ebreak opcodes
			gen_ebreak=true;
			i++;
		} else if (strcmp(argv[i],"-ecall")==0) {					// Disable generating ecall opcodes
			gen_ecall=true;
			i++;
	
		} else if (strcmp(argv[i],"-d")==0) {						// Debug Mode
			if ((argc-1)>i) debug=debug | (int)strtol(argv[++i], NULL, 16);	// debug in hex! 		
				else usage_exit();			  
			i++;
			
		} else if (strcmp(argv[i],"-ic")==0) {						// Set instruction count, default to 512K
			if ((argc-1)>i) {
				icnt_max=(uint32_t)strtol(argv[++i],NULL,16);		// Length in hex
			} else usage_exit();			  
			i++;

		} else if (strcmp(argv[i],"-sb")==0) {						// Set memory read/write base address
			if ((argc-1)>i) {
				base_address=(uint32_t)strtol(argv[++i],NULL,16);	// in hex
			} else usage_exit();			  
			i++;

		} else if(strcmp(argv[i],"-s")==0) {                		// Change Seed, default to 1
            if ((argc-1)>=i) {
				seed=strtol(argv[++i],NULL,0);    
			} else usage_exit();            
            i++;

		} else if (strcmp(argv[i],"-march")==0) {					// Set Memory Architecture
			if ((argc-1)>i) {
				strncpy(march,argv[++i],sizeof(march));
				if (debug) printf("New march=%s \n",march);
			} else usage_exit();			  
			i++;			
	
        } else {
            if ((argc-1)>=i && argv[i][0]!='-') {
				strcpy(asmfilename,argv[i]); 
            } else usage_exit();
			i++;
		}	
	} 
   
	qprintf("\n***********************************************************\n");
	qprintf("*** RV32GEN: Risc-V Pseudo Random Instruction Generator ***\n");
	qprintf("***               Ver %d.%d (c)2022 HT-LAB                ***\n", MAJOR_VERSION,MINOR_VERSION);
	qprintf("***********************************************************\n");
	qprintf("ebreak generation  : %s\n",gen_ebreak ? "Enabled" : "Disabled");
	qprintf("ecall generation   : %s\n",gen_ecall ? "Enabled" : "Disabled");
	qprintf("Generate           : 0x%x(%d) instructions\n",icnt_max,icnt_max);
	qprintf("Random Seed        : %d\n",seed);
	qprintf("Output Filename    : %s\n",asmfilename);
	qprintf("Architecture       : %s\n\n",march);
	
	if ((fp=fopen(asmfilename,"w"))==NULL) {
		printf("\nFailed to open output file %s\n",asmfilename);
		return 1;
	}	
	
	//fputs("\n.option norvc\n",fp);
	
	fprintf(fp,"# RV32GEN, ver %d.%d\n", MAJOR_VERSION,MINOR_VERSION);
	fprintf(fp,"# march=%s\n",march);
	fprintf(fp,"# ebreak is %s\n",gen_ebreak ? "Included" : "Excluded");
	fprintf(fp,"# ecall is %s\n",gen_ecall ? "Included" : "Excluded");
	fprintf(fp,"# Seed used %d\n",seed);
	fprintf(fp,"\nmain:\t%s # Start Pseudo Random Instruction Test\n",gen_bb(buf,NOEXCL));
	icnt++;
	
	srand(seed);													// Can change with -s command
	
	do {
		opc=LUI+random(INVALID);
		buf[0]='\0';
		switch (opc) {
			//-------------------------------------------------------------------------------------
			//			ORI		s8,a4, 29		# random
			// 			JAL 	regx,jf001		# Test forward
			//			XORI	s5,s4,-397		# random
			//			J       js001
			// jf001:	JALR	regx
			// js001:	
			//
			//          J       jf002
			// jb002:	JALR	regx
			// jf002:   ORI		s8,a4, 29		# random
			//          JAL		regx,jb002		# Test Backwards
			//          						# destination of regx
			//-------------------------------------------------------------------------------------
			case JAL    : 
				linkreg=random(32);
				if (linkreg==0) linkreg+=1;							// avoid x0
				if (random(2)) {
					fprintf(fp,"\t\t%-6.6s\t%s,jf%d\t# Test jump forwards\n",opc32Is[opc],regs[linkreg],labelcnt);
					fprintf(fp,"\t\t%s\n",gen_bb(buf,NOEXCL));
					fprintf(fp,"\t\tJ\t\tjs%d\n",labelcnt);
					fprintf(fp,"jf%d:\tJALR\t%s\n",labelcnt,regs[linkreg]);
					fprintf(fp,"js%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					icnt+=5;
				} else {
					fprintf(fp,"\t\tJ\t\tjf%d\n",labelcnt);
					fprintf(fp,"jb%d:\tJALR\t%s\n",labelcnt,regs[linkreg]);
					fprintf(fp,"jf%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					fprintf(fp,"\t\t%-6.6s\t%s,jb%d\t# Test jump backwards\n",opc32Is[opc],regs[linkreg],labelcnt);
					icnt+=4;
				}
				labelcnt++;
				break;
				
			//-------------------------------------------------------------------------------------
			// ja001:	AUIPC   regx,%hi(jf001) # get jf001 address
			// 			ADDI    regx,regx,%lo(ja001)     
			//			SUB		regx,offset		# subtract or add random +/-2K value
			//			XORI	s5,s4,-397		# random not modifying regx
			// 			JALR 	t4,offset(regx) # Jump forward, regx+/offset is jf001 address
			//			ORI		s8,a4, 29		# random
			// jf001:			
			//
			// ja002:	AUIPC   regx,%hi(jb002) # get jb002 address
			// 			ADDI    regx,regx,%lo(ja002)   
			//			ADDI	regx,offset		# subtract or add random +/-2K value			
			//          J       jf002
			// jb002:	XORI	s5,s4,-397		# random 
			// 			J       js002
			// jf002:   ORI		s8,a4, 29		# random not modifying regx
			//			JALR 	t4,offset(regx) # Jump Backwards, regx+/offset is jb002 address
			//			ORI		s8,a4, 29		# random
			// js002:	
			//-------------------------------------------------------------------------------------	
			case JALR   : 
				linkreg=random(32);
				if (linkreg==0) linkreg+=1;							// avoid x0
				offset=random(4095)-2047;
				if (random(2)) {
					fprintf(fp,"ja%d:\tAUIPC\t%s,%%pcrel_hi(jf%d)\n",labelcnt,regs[linkreg],labelcnt);
					fprintf(fp,"\t\tADDI\t%s,%s,%%pcrel_lo(ja%d)\n",regs[linkreg],regs[linkreg],labelcnt); 
					fprintf(fp,"\t\tADDI\t%s,%s,%d\n",regs[linkreg],regs[linkreg],offset); 
					fprintf(fp,"\t\t%s\n",gen_bb(buf,linkreg));
					fprintf(fp,"\t\tJALR\t%s,%d(%s) # Jump Forward\n",regs[linkreg],~offset+1,regs[linkreg]);
					fprintf(fp,"\t\t%s\n",gen_bb(buf,NOEXCL));
					fprintf(fp,"jf%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					icnt+=7;
				} else {
					fprintf(fp,"ja%d:\tAUIPC\t%s,%%pcrel_hi(jb%d)\n",labelcnt,regs[linkreg],labelcnt);
					fprintf(fp,"\t\tADDI\t%s,%s,%%pcrel_lo(ja%d)\n",regs[linkreg],regs[linkreg],labelcnt); 	
					fprintf(fp,"\t\tADDI\t%s,%s,%d\n",regs[linkreg],regs[linkreg],offset);					
					fprintf(fp,"\t\tJ\t\tjf%d\n",labelcnt);
					fprintf(fp,"jb%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					fprintf(fp,"\t\tJ\t\tjs%d\n",labelcnt);
					fprintf(fp,"jf%d:\t%s\n",labelcnt,gen_bb(buf,linkreg));
					fprintf(fp,"\t\tJALR\t%s,%d(%s) # Jump Backwards\n",regs[linkreg],~offset+1,regs[linkreg]);
					fprintf(fp,"\t\t%s\n",gen_bb(buf,NOEXCL));
					fprintf(fp,"js%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					icnt+=10;
				}
				labelcnt++;				
				break;
			
			//-------------------------------------------------------------------------------------
			// 			J 		jf001
			// jb001:	XORI	s5,s4,-397
			// 			J 		js001
			// jf001: 	XOR		gp,t1,s11		# random
			// 			beqz 	a5,jb001		# Instruction tested jump backward
			// js001: 	ORI		s8,a4, 29		# random
			//
			//			XOR		gp,t1,s11		# random
			// 			beqz	a5,jf002		# Instruction tested jump forward
			// 			SLL		t2,t3,a7		# random
			// jf002:
			//-------------------------------------------------------------------------------------
			case BEQ    :
			case BNE    :
			case BLT    :
			case BGE    :
			case BLTU   :
			case BGEU   : 
				if (random(2)) {									// Jump forward
					fprintf(fp,"\t\tJ\t\tjf%d\n",labelcnt);
					fprintf(fp,"jb%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					fprintf(fp,"\t\tJ\t\tjs%d\n",labelcnt);
					fprintf(fp,"jf%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					fprintf(fp,"\t\t%-6.6s\t%s,%s,jb%d\t# Test jump backwards\n",opc32Is[opc],regs[random(32)],regs[random(32)],labelcnt);
					fprintf(fp,"js%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					icnt+=6;					
				} else {											// Jump backward
					fprintf(fp,"\t\t%s\n",gen_bb(buf,NOEXCL));
					fprintf(fp,"\t\t%-6.6s\t%s,%s,jf%d\t# Test jump forwards\n",opc32Is[opc],regs[random(32)],regs[random(32)],labelcnt);
					fprintf(fp,"\t\t%s\n",gen_bb(buf,NOEXCL));
					fprintf(fp,"jf%d:\t%s\n",labelcnt,gen_bb(buf,NOEXCL));
					icnt+=4;
				}
				labelcnt++;
				break;
							
							
			//-------------------------------------------------------------------------------------		
			// Restrict I/O to a particular range base_address +/- 2047
			// 			LUI 		regx,BASE_ADDRESS>>12; 	
			//			XOR			gp,t1,s11					# random			
			//			L/S   		s0,+/-offset(regx)
			//			SLL			t2,t3,a7					# random
			//
			// 			SB,LB(U)   	s0,+/-offset(regx)
			//			SH,LH(U)   	s0,+/-(offset&0xFFE)(regx)
			//			SW,LW      	s0,+/-(offset&0xFFC)(regx)	
			//-------------------------------------------------------------------------------------
			case LB     :
			case LBU    :
			case SB     :
			case LH     :
			case LHU    :
			case SH     :			
			case LW     :			
			case SW     :
				linkreg=random(32);
				if (linkreg==0) linkreg+=1;								// avoid x0
				offset=random(4095)-2047;
				if (opc==LW || opc==SW) offset&=0xFFFFFFFC;				// Change if memory exceptions are allowed
				if (opc==LH || opc==LHU || opc==SH) offset&=0xFFFFFFFE;	// Change if memory exceptions are allowed					
				fprintf(fp,"\t\tLUI\t\t%s,%d>>12\t# Load/Store base address\n",regs[linkreg],base_address); 	
				fprintf(fp,"\t\t%s\n",gen_bb(buf,linkreg));
				fprintf(fp,"\t\t%-6.6s\t%s,%d(%s)\n",opc32Is[opc],regs[random(32)],offset,regs[linkreg]);
				icnt+=3;				
				break;
											
			case FENCE  : if (debug)      fprintf(fp,"# %s\n",opc32Is[opc]); break;
			case ECALL  : if (gen_ecall)  {fprintf(fp,"\t\t%s\n",opc32Is[opc]);icnt++;} break;
			case EBREAK : if (gen_ebreak) {fprintf(fp,"\t\t%s\n",opc32Is[opc]);icnt++;} break;				

			default     : 
				fprintf(fp,"\t\t%s\n",gen_bb(buf,NOEXCL)); 
				icnt++;			
		}
		
		
	} while (icnt<icnt_max);
	
	fprintf(fp,"\t\tebreak  # End of Test\n");
	
	fclose(fp);
	return 0;
}

//-------------------------------------------------------------------------------------------------
// Generate safe non-branch instructions similar to a "basic block" used in compilers
//-------------------------------------------------------------------------------------------------
char *gen_bb(char *buf,int excl_reg)
{
	enum opc32I opc;
	int  rd=0;
	int  rs1=random(32);
	int  rs2=random(32);
	
	do {															// Check if we need to exclude reg
		rd=random(32);
	} while (rd==excl_reg);
	
	do {
		opc=LUI+random(INVALID);
		buf[0]='\0';
		switch (opc) {
			case LUI    :  			
			case AUIPC  : sprintf(buf,"%-6.6s\t%s,0x%x",opc32Is[opc],regs[rd],random(1048576)); break;
							
			case ADDI   :
			case SLTI   :
			case SLTIU  :
			case XORI   :
			case ORI    :
			case ANDI   : sprintf(buf,"%-6.6s\t%s,%s,%c%d",opc32Is[opc],regs[rd],regs[rs1],random(2)?'-':' ',random(2047)); break;
			
			case SLLI   :
			case SRLI   :
			case SRAI   : sprintf(buf,"%-6.6s\t%s,%s,%d",opc32Is[opc],regs[rd],regs[rs1],random(32)); break;
			
			case ADD    :
			case SUB    :
			case SLL    :
			case SLT    :
			case SLTU   :
			case XOR    :
			case SRL    :
			case SRA    :
			case OR     :
			case AND    : sprintf(buf,"%-6.6s\t%s,%s,%s",opc32Is[opc],regs[rd],regs[rs2],regs[rs1]); break;
			
			default:
				if ((strchr(march,'m') != NULL) || (strchr(march,'M') != NULL))  {
					switch (opc) {
						case MUL   :
						case MULH  :
						case MULHSU:
						case MULHU :
						case DIV   :
						case DIVU  :
						case REM   :
						case REMU  :
							sprintf(buf,"%-6.6s\t%s,%s,%s",opc32Is[opc],regs[rd],regs[rs2],regs[rs1]); break;
						default:;
					}
				}
		}
	} while (strlen(buf)==0);	
	
	return buf;
}


uint32_t random(uint32_t mul)
{
    return (uint32_t) (((float)mul*rand())/(RAND_MAX+1.0));
}

void usage_exit(void)
{
   	printf("\n*** RV32GEN Ver %d.%d (c)2022 HT-LAB ***\n", MAJOR_VERSION,MINOR_VERSION);    
	printf("\nUsage               : genriscv <options> <output_filename> \n");
    printf("\nOptions:\n");
    printf("-q                  : Quiet, must be specified first in the options list\n");	
	printf("-s <integer>        : Set randomize seed, default seed=time()\n");
	printf("-march <string>     : Generate instructions for architecture, default to \"IMC\"\n");	
	printf("-ebreak             : include ebreak opcodes, default excluded\n");
	printf("-ecall              : include ecall opcodes, default excluded\n");
	printf("-ic <hex>           : Instruction Count, default to 512 (2KByte)\n");
	printf("-sb <hex>           : Set load/store Base address, default to 0x1000\n");	
    exit(1);
}
