#ifndef _myTables_h
#define _myTables_h

// Interpolating, clipping, 1 and 2-D arbitrarily spaced table look-up

void binsearch(double x, double *v, int n, int *high, int *low, double *dx);
double tab1(double x, double *v, double *y, int n);
double tab1clip(double x, double *v, double *y, int n);
double tab2(double x1, double x2, double *v1, double *v2, double *y, int n1, int n2);

class TableInterp
{
public:
  TableInterp();
  TableInterp(const unsigned int n, const double x[]);
  TableInterp(const unsigned int n, const float x[]);   // TODO:  template for tables
  virtual ~TableInterp();
  // operators
  // functions
  virtual double interp(void);
  void pretty_print(void);
protected:
  unsigned int n1_;
  double *x_;
  double *v_;
};

// 1-D Interpolation Table Lookup
class TableInterp1D : public TableInterp
{
public:
  TableInterp1D();
  TableInterp1D(const unsigned int n, const double x[], const double v[]);
  TableInterp1D(const unsigned int n, const float x[], const float v[]);  // TODO:  template
  ~TableInterp1D();
  //operators
  //functions
  virtual double interp(const double x);

protected:
};

// 1-D Interpolation Table Lookup with Clipping
class TableInterp1Dclip : public TableInterp
{
public:
  TableInterp1Dclip();
  TableInterp1Dclip(const unsigned int n, const double x[], const double v[]);
  ~TableInterp1Dclip();
  //operators
  //functions
  virtual double interp(const double x);

protected:
};

// 2-D Interpolation Table Lookup
class TableInterp2D : public TableInterp
{
public:
  TableInterp2D();
  TableInterp2D(const unsigned int n, const unsigned int m, const double x[],
                const double y[], const double v[]);
  TableInterp2D(const unsigned int n, const unsigned int m, const float x[],
                const float y[], const float v[]);
  ~TableInterp2D();
  //operators
  //functions
  virtual double interp(const double x, const double y);

protected:
  unsigned int n2_;
  double *y_;
};

#endif

