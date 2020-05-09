// #include <sysutil.h>
int regs[12];
char str[13];
main()
{
    ucsinitreg(regs);
    printf("Initial Register Contents\n");
    printf("A2/A3 : %012o %012o\n",regs[0],regs[1]);
    printf("A4 : %012o\n",regs[2]);
    printf("A5 : %012o\n",regs[3]);
    fdasc(str,&regs[4],6);
    str[7] = '\0';
    printf("R1 : %012o (%s)\n",regs[4],str);
    printf("R2 : %012o\n",regs[5]);
    printf("R3 : %012o\n",regs[6]);
    fdasc(str,&regs[7],12);
    str[13] = '\0';
    printf("R11/R12: %012o %012o (%s)\n",regs[7],regs[8],str);
    fdasc(str,&regs[9],12);
    str[13] = '\0';
    printf("R13/R14: %012o %012o (%s)\n",regs[9],regs[10],str);
    fdasc(str,&regs[11],6);
    str[7] = '\0';
    printf("R15 : %012o (%s)\n",regs[11],str);
}
