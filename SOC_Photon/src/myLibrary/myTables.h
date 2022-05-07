#ifndef _myTables_h
#define _myTables_h
#define t_float double

// Interpolating, clipping, 1 and 2-D arbitrarily spaced table look-up

void binsearch(t_float x, t_float *v, int n, int *high, int *low, t_float *dx);
t_float tab1(t_float x, t_float *v, t_float *y, int n);
t_float tab1clip(t_float x, t_float *v, t_float *y, int n);
t_float tab2(t_float x1, t_float x2, t_float *v1, t_float *v2, t_float *y, int n1, int n2);

class TableInterp
{
public:
  TableInterp();
  TableInterp(const unsigned int n, const t_float x[]);   // TODO:  template for tables
  virtual ~TableInterp();
  // operators
  // functions
  virtual t_float interp(void);
  void pretty_print(void);
protected:
  unsigned int n1_;
  t_float *x_;
  t_float *v_;
};

// 1-D Interpolation Table Lookup
class TableInterp1D : public TableInterp
{
public:
  TableInterp1D();
  TableInterp1D(const unsigned int n, const t_float x[], const t_float v[]);  // TODO:  template
  ~TableInterp1D();
  //operators
  //functions
  virtual t_float interp(const t_float x);

protected:
};

// 1-D Interpolation Table Lookup with Clipping
class TableInterp1Dclip : public TableInterp
{
public:
  TableInterp1Dclip();
  TableInterp1Dclip(const unsigned int n, const t_float x[], const t_float v[]);
  ~TableInterp1Dclip();
  //operators
  //functions
  virtual t_float interp(const t_float x);

protected:
};

// 2-D Interpolation Table Lookup
class TableInterp2D : public TableInterp
{
public:
  TableInterp2D();
  TableInterp2D(const unsigned int n, const unsigned int m, const t_float x[],
                const t_float y[], const t_float v[]);
  ~TableInterp2D();
  //operators
  //functions
  virtual t_float interp(const t_float x, const t_float y);

protected:
  unsigned int n2_;
  t_float *y_;
};

#endif

