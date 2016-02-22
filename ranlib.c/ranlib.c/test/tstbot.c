#include "ranlib.h"
#include <stdio.h>
#include <stdlib.h>
void main(int argc,char argv[])
/*
**********************************************************************
     A test program for the bottom level routines
**********************************************************************
     Set up the random number generator
*/
{
static long genlst[5] = {
    1,5,10,20,32
};
static long ians,iblock,igen,iseed1,iseed2,itmp,ix,ixgen,nbad,answer[10000];

    nbad = 0;
    puts(" For five virual generators of the 32");
    puts(" This test generates 10000 numbers then resets the block");
    puts("      and does it again");
    puts(" Any disagreements are reported -- there should be none");
/*
     Set up Generators
*/
    setall(12345L,54321L);
/*
     For a selected set of generators
*/
    for(ixgen=0; ixgen<5; ixgen++) {
        igen = *(genlst+ixgen);
        gscgn(1L,&igen);
        printf(" Testing generator %12ld\n",igen);
/*
     Use 10 blocks
*/
        initgn(-1L);
        getsd(&iseed1,&iseed2);
        for(iblock=0; iblock<10; iblock++) {
/*
     Generate 1000 numbers
*/
            for(ians=1; ians<=1000; ians++) {
                ix = ians+iblock*1000;
                *(answer+ix-1) = ignlgi();
            }
            initgn(1L);
        }
        initgn(-1L);
/*
     Do it again and compare answers
*/
        getsd(&iseed1,&iseed2);
/*
     Use 10 blocks
*/
        for(iblock=1; iblock<=10; iblock++) {
/*
     Generate 1000 numbers
*/
            for(ians=1; ians<=1000; ians++) {
                ix = ians+(iblock-1)*1000;
/*
      ANSWER( IX ) = IGNLGI()
*/
                itmp = ignlgi();
                if((itmp != *(answer+ix-1))) {
                puts(" Disagreement on regeneration of numbers");
                printf(" Block %2ld N within Block %2ld\n",&iblock,&ians);
                printf(" Index in answer %5ld\n", &ix);
                printf(" Originally Generated %10ld Regenerated %10ld\n",
                       (answer+ix-1),&itmp);
                nbad += 1;
                if(nbad > 10) {
                    puts(" More than 10 mismatches - ABORT");
                       exit(1);
                    }     
              }
              }
            initgn(1L);
          }
        printf(" Finished testing generator %12ld\n",igen);
        puts(" Test completed successfully");
    }
    exit(0);
}
