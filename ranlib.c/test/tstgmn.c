#include "ranlib.h"
#include <stdio.h>
#include <math.h>

extern float covar(float *x,float *y,long n);
extern void prcomp(long p,float *mean,float *xcovar,float *answer);
extern void setcov(long p,float *var,float corr,float *covar);
extern void statist(float *x,long *n,float *av,float *var,
                 float *xmin,float *xmax);

float covar(float *x,float *y,long n)
{
static long i;
static float covar,avx,avy,varx,vary,xmax,xmin;

    statist(x,&n,&avx,&varx,&xmin,&xmax);
    statist(y,&n,&avy,&vary,&xmin,&xmax);
    covar = 0.0;
    for(i=0; i<n; i++) covar += ((*(x+i)-avx)*(*(y+i)-avy));
    covar /= (float)(n-1);
    return covar;
}
void prcomp(long p,float *mean,float *xcovar,float *answer)
{
#define maxp 10L
#define maxobs 1000L
static long i,T1,j,D2,D3;
static float rcovar[100],rmean[10],rvar[10],dum1,dum2;
    for(i=1; i<=p; i++) {
        T1 = maxobs;
        statist((answer+(i-1)*1000L),&T1,(rmean+i-1),(rvar+i-1),&dum1,&dum2);
        printf(" Variable Number%12ld\n",i);
        printf(" Mean %16.6g Generated %16.6g\n",*(mean+i-1),*(rmean+i-1));
        printf(" Variance %16.6g Generated%16.6g\n",*(xcovar+i-1+(i-1)*p),*
          (rvar+i-1));
    }
    puts("                   Covariances");
    for(i=1; i<=p; i++) {
        for(j=1,D2=1,D3=(i-1-j+D2)/D2; D3>0; D3--,j+=D2) {
            printf(" I = %12ld J = %12ld\n",i,j);
            *(rcovar+i-1+(j-1)*10) = covar((answer+(i-1)*1000L),(answer+(j-1)*
              1000L),maxobs);
            printf(" Covariance %16.6g Generated %16.6g\n",*(xcovar+i-1+(j-1)*p)
              ,*(rcovar+i-1+(j-1)*10));
        }
    }
#undef maxp
#undef maxobs
}
void setcov(long p,float *var,float corr,float *covar)
/*
     Set covariance matrix from variance and common correlation
*/
{
static long i,j,D1,D2;

    for(i=1,D1=1,D2=(p-i+D1)/D1; D2>0; D2--,i+=D1) {
        for(j=1; j<=p; j++) {
            if(!(i == j)) goto S10;
            *(covar+i-1+(j-1)*p) = *(var+i-1);
            goto S20;
S10:
            *(covar+i-1+(j-1)*p) = corr*sqrt(*(var+i-1)**(var+j-1));
S20:
            continue;
        }
    }
}
void statist(float *x,long *n,float *av,float *var,float *xmin,float *xmax)
{
static long i;
static float sum;

    *xmin = *xmax = *x;
    sum = 0.0;
    for(i=0; i<*n; i++) {
        sum += *(x+i);
        if(*(x+i) < *xmin) *xmin = *(x+i);
        if(*(x+i) > *xmax) *xmax = *(x+i);
    }
    *av = sum/(float)*n;
    sum = 0.0;
    for(i=0; i<*n; i++) sum += pow(*(x+i)-*av,2.0);
    *var = sum/(float)(*n-1);
}
void main(long argc,char *argv[])
{
#define maxp 10L
#define maxobs 1000L
#define p2 100L
static long i,iobs,is1,is2,j,p;
static float corr,answer[10000],ccovar[100],covar[100],mean[10],param[500],
    temp[10],var[10],work[10];
static char phrase[100];

    puts(" Tests Multivariate Normal Generator for Up to 10 Variables");
    puts(" User inputs means, variances, one correlation that is applied");
    puts("     to all pairs of variables");
    puts(" 1000 multivariate normal deviates are generated");
    puts(" Means, variances and covariances are calculated for these");
S10:
    puts("Enter number of variables for normal generator");
    scanf("%d",&p);
    printf("Enter mean vector of length %12ld\n",p);
    for(i=0; i<p; i++) scanf("%f",(mean+i));
    printf("Enter variance vector of length %12ld\n",p);
    for(i=0; i<p; i++) scanf("%f",(var+i));
    puts("Enter correlation of all variables");
    scanf("%f",&corr);
    setcov(p,var,corr,covar);
    puts(" Enter phrase to initialize rn generator");
    scanf("%s",phrase);
    phrtsd(phrase,&is1,&is2);
    setall(is1,is2);
    for(i=0; i<p2; i++) *(ccovar+i) = *(covar+i);
/*
     Generate Variables
*/
    setgmn(mean,ccovar,p,param);
    for(iobs=0; iobs<maxobs; iobs++) {
        genmn(param,work,temp);
        for(j=0; j<p; j++) *(answer+iobs+j*1000L) = *(work+j);
    }
    prcomp(p,mean,covar,answer);
/*
     Print [004qComparison of Generated and Reconstructed Values
*/
    goto S10;
#undef maxp
#undef maxobs
#undef p2
}
