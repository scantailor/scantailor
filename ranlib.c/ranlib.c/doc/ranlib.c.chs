










                                    RANLIB.C

            Library of C Routines for Random Number Generation








                     Summary Documentation of Each Routine








                            Compiled and Written by:

                                 Barry W. Brown
                                  James Lovato
                                   









                     Department of Biomathematics, Box 237
                     The University of Texas, M.D. Anderson Cancer Center
                     1515 Holcombe Boulevard
                     Houston, TX      77030


 This work was supported by grant CA-16672 from the National Cancer Institute.





                    SUMMARY OF ROUTINES IN RANLIB

0. Base Level Routines to Set and Obtain Values of Seeds

(These should be the only base level routines used by  those who don't
need multiple generators with blocks of numbers.)

**********************************************************************

     extern void setall(long iseed1,long iseed2);
               SET ALL random number generators

**********************************************************************
**********************************************************************

     extern void getsd(long *iseed1,long *iseed2)
               GET SeeD

     Returns the value of two integer seeds of the current generator
     in ISEED1, ISEED2

**********************************************************************

I. Higher Level Routines

**********************************************************************

     extern float genbet(float aa,float bb);
               GeNerate BETa random deviate

     Returns a single random deviate from the beta distribution with
     parameters A and B.  The density of the beta is
               x^(a-1) * (1-x)^(b-1) / B(a,b) for 0 < x < 1

**********************************************************************
**********************************************************************

     extern float genchi(float df);
                Generate random value of CHIsquare variable

     Generates random deviate from the distribution of a chisquare
     with DF degrees of freedom random variable.

**********************************************************************
**********************************************************************

     extern float genexp(float av);
                    GENerate EXPonential random deviate

     Generates a single random deviate from an exponential
     distribution with mean AV.

**********************************************************************




**********************************************************************

     extern float genf(float dfn, float dfd);
                GENerate random deviate from the F distribution

     Generates a random deviate from the F (variance ratio)
     distribution with DFN degrees of freedom in the numerator
     and DFD degrees of freedom in the denominator.

**********************************************************************
**********************************************************************

     extern float gengam(float a,float r);
           GENerates random deviates from GAMma distribution

     Generates random deviates from the gamma distribution whose
     density is
          (A**R)/Gamma(R) * X**(R-1) * Exp(-A*X)

**********************************************************************
**********************************************************************

     
     extern void genmn(float *parm,float *x,float *work);
         GENerate Multivariate Normal deviate

     Generates a multivariate   normal deviate   (parameters  of  which
     were  previously  set in   parm    by setgmn).   Returns    random
     deviate of length p in x; work is a work array of length p.
     
**********************************************************************
**********************************************************************

     extern float gennch(float df,float xnonc);
           Generate random value of Noncentral CHIsquare variable

     Generates random deviate  from the  distribution  of a  noncentral
     chisquare with DF degrees  of freedom and noncentrality  parameter
     XNONC.

**********************************************************************
**********************************************************************

     extern void genmul(long n,float *p,long ncat,long *ix);
              GENerate MULtinomial random deviate

     Generates deviates from a Multinomial distribution with NCAT
     categories.  P specifies the probability of an event in each
     category. The generated deviates are placed in IX.

**********************************************************************
**********************************************************************

     extern float gennf(float dfn, float dfd, float xnonc);
           GENerate random deviate from the Noncentral F distribution

     Generates a random deviate from the  noncentral F (variance ratio)
     distribution with DFN degrees of freedom in the numerator, and DFD
     degrees of freedom in the denominator, and noncentrality parameter
     XNONC.

**********************************************************************




**********************************************************************

     extern float gennor(float av,float sd);
         GENerate random deviate from a NORmal distribution

     Generates a single random deviate from a normal distribution
     with mean, AV, and standard deviation, SD.

**********************************************************************
**********************************************************************

    extern void genprm(long *iarray,int larray);
               GENerate random PeRMutation of iarray

**********************************************************************
**********************************************************************

     extern float genunf(float low,float high);
               GeNerate Uniform Real between LOW and HIGH

**********************************************************************
**********************************************************************

     extern long ignbin(long n,float pp);
                    GENerate BINomial random deviate

     Returns a single random deviate from a binomial
     distribution whose number of trials is N and whose
     probability of an event in each trial is P.

**********************************************************************
**********************************************************************
     extern long ignnbn(long n,float p);
               GENerate Negative BiNomial random deviate

     Returns a single random deviate from a negative binomial
     distribution with number of events N and whose
     probability of an event in each trial is P.

**********************************************************************
**********************************************************************

     extern long ignpoi(float mu);
                    GENerate POIsson random deviate

     Generates a single random deviate from a Poisson
     distribution with mean AV.

**********************************************************************
**********************************************************************

     extern long ignuin(long low,long high);
               GeNerate Uniform INteger

     Generates an integer uniformly distributed between LOW and HIGH.

**********************************************************************
**********************************************************************

     extern void phrtsd(char* phrase,long* seed1,long* seed2);
               PHRase To SeeDs
     CHARACTER*(*) PHRASE

     Uses a phrase (character string) to generate two seeds for the RGN
     random number generator.

**********************************************************************




**********************************************************************

     extern float ranf(void);
                RANDom number generator as a Function

     Returns a random floating point number from a uniform distribution
     over 0 - 1 (endpoints of this interval are not returned) using the
     current generator

**********************************************************************
**********************************************************************

    extern void setgmn(float *meanv,float *covm,long p,float *parm);
            SET Generate Multivariate Normal random deviate

     P is the length of normal vectors to be generated, MEANV
     is the vector of their means and COVM is their variance
     covariance matrix.  Places information necessary to generate
     the deviates in PARM.
     (Parm must have length p*(p+3)/2 + 1)
**********************************************************************

II. Uniform Generator and Associated Routines


      A. SETTING THE SEED OF ALL GENERATORS

**********************************************************************

      extern void setall(long iseed1,long iseed2);
               SET ALL random number generators

**********************************************************************

      B. OBTAINING RANDOM NUMBERS

**********************************************************************

     extern long ignlgi(void);
               GeNerate LarGe Integer

     Returns a random integer following a uniform distribution over
     (1, 2147483562) using the current generator.

**********************************************************************

**********************************************************************

     
     extern float ranf(void);
                RANDom number generator as a Function

     Returns a random floating point number from a uniform distribution
     over 0 - 1 (endpoints of this interval are not returned) using the
     current generator

**********************************************************************




      C. SETTING AND OBTAINING THE NUMBER OF THE CURRENT GENERATOR

**********************************************************************
     void gscgn(long getset,long *g)
                         Get/Set GeNerator
     Gets or returns in G the number of the current generator
                              Arguments
     getset --> 0 Get
                1 Set
     g <--> Number of the current random number generator (1..32)
**********************************************************************
**********************************************************************

      D. OBTAINING OR CHANGING SEEDS IN CURRENT GENERATOR

**********************************************************************

    extern void advnst(long k);
               ADV-a-N-ce ST-ate

     Advances the state  of  the current  generator  by 2^K values  and
     resets the initial seed to that value.

**********************************************************************

**********************************************************************

     extern void getsd(long *iseed1,long *iseed2);
               GET SeeD

     Returns the value of two integer seeds of the current generator
     in ISEED1, ISEED2

**********************************************************************

**********************************************************************

     extern void initgn(long isdtyp);
          INIT-ialize current G-e-N-erator

          ISDTYP = -1  => sets the seeds to their initial value
          ISDTYP =  0  => sets the seeds to the first value of
                          the current block
          ISDTYP =  1  => sets the seeds to the first value of
                          the next block

**********************************************************************

**********************************************************************

     extern void setsd(long iseed1,long iseed2);
               SET S-ee-D of current generator

     Resets the initial  seed of  the current  generator to  ISEED1 and
     ISEED2. The seeds of the other generators remain unchanged.

**********************************************************************





      E. MISCELLANY

**********************************************************************

     extern long mltmod(long a,long s,long m);
                    Returns (A*S) MOD M

**********************************************************************

**********************************************************************

      extern void setant(long qvalue);
               SET ANTithetic
      LOGICAL QVALUE

     Sets whether the current generator produces antithetic values.  If
     X   is  the value  normally returned  from  a uniform [0,1] random
     number generator then 1  - X is the antithetic  value. If X is the
     value  normally  returned  from a   uniform  [0,N]  random  number
     generator then N - 1 - X is the antithetic value.

     All generators are initialized to NOT generate antithetic values.

**********************************************************************
