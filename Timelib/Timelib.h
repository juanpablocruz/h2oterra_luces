#ifndef _TIMELIB_H_
#define _TIMELIB_H_


typedef struct Hora{
   int h_inicio,h_final,m_inicio,m_final, s_inicio, s_final; 
};

typedef struct Calendario{
   Hora h;
   int lenDias;
   int *Dias;
   int motor;
   int tipo; 
};


#endif