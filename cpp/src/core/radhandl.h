/*-------------------------------------------------------------------------
*
* File name:      radhandl.h
*
* Project:        RADIA
*
* Description:    Smart pointer for RADIA objects
*
* Author(s):      Oleg Chubar, Pascal Elleaume
*
* First release:  1997
*
* Copyright (C):  1997 by European Synchrotron Radiation Facility, France
*                 All Rights Reserved
*
-------------------------------------------------------------------------*/

#ifndef __RADHANDLE_H
#define __RADHANDLE_H

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

template<class T> class radTHandle {
public:
	T* rep;
	int* pcount;

	radTHandle () { rep=0; pcount=0;}
	radTHandle (T* pp) : rep(pp), pcount(new int) { /* (*pcount)=0; */ (*pcount)=1;}
	radTHandle (const radTHandle& r) : rep(r.rep), pcount(r.pcount)
	{
#pragma omp critical(init_copy)
          {
		if(pcount != 0) (*pcount)++;
          }
	}

	void destroy()
	{
          T* rep_to_delete = 0;
          int* pcount_to_delete = 0;
#pragma omp critical(destroy)
          {
		if(pcount!=0)
			if(--(*pcount)==0)
			{
                                rep_to_delete = rep;
                                pcount_to_delete = pcount;
				rep=0; pcount=0;
			}
          }
          delete rep_to_delete;
          delete pcount_to_delete;
	}

	T* operator->() {
          T* res;
#pragma omp atomic read
          res = rep;
          return res;
        }
	T* obj() {
          T* res;
#pragma omp atomic read
          res = rep;
          return res;
        }
	void bind(const radTHandle& r)
	{
          T* rep_to_delete = 0;
          int* pcount_to_delete = 0;
#pragma omp critical(bind)
          {
		if(rep!=r.rep)
		{
			if(r.rep!=0)
			{
                          if(pcount!=0)
                            if(--(*pcount)==0)
                              {
                                rep_to_delete = rep;
                                pcount_to_delete = pcount;
                              }
				rep = r.rep;
				pcount = r.pcount;
				(*pcount)++;
			}
			else
			{
				rep = 0;
				pcount = 0;
			}
		}
          }
          delete rep_to_delete;
          delete pcount_to_delete;
	}

	radTHandle& operator=(const radTHandle& r)
	{
		bind(r); return *this;
	}

	int operator<(const radTHandle& r)
	{
          int res;
#pragma omp critical(less_than)
          {
            res = rep<r.rep ? 1 : 0;
          }
          return res;

	}
	int operator==(const radTHandle& r)
	{
          int res;
#pragma omp critical(equals)
          {
            res = rep==r.rep ? 1 : 0;
          }
          return res;
	}

	~radTHandle()
	{
		destroy();
	}
};

//-------------------------------------------------------------------------

template<class T> inline int operator <(const radTHandle<T>& h1, const radTHandle<T>& h2)
{
  int res;
#pragma omp critical(less_than_op)
  {
  res = h1.rep < h2.rep;
  }
  return res;
}

//-------------------------------------------------------------------------

template<class T> inline int operator ==(const radTHandle<T>& h1, const radTHandle<T>& h2)
{
  int res;
#pragma omp critical(equals_op)
  {
  res = h1.rep == h2.rep;
  }
  return res;
}

//-------------------------------------------------------------------------

#endif
