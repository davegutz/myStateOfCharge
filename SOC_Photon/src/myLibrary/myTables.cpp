#include <Arduino.h> //needed for Serial.println
#include "myTables.h"
#include "math.h"

// Global variables
extern char buffer[256];

// Interpolating, clipping, 1 and 2-D arbitrarily spaced table look-up

/* B I N S E A R C H
*
*   Purpose:    Find x in { v[0] <= v[1] <= ... ,= v[n-1] } and calculate
*               the fraction of range that x is positioned.
*
*   Author:     Dave Gutz 09-Jun-90.
*   Revisions:  Dave Gutz 20-Aug-90 Input x instead of *x to protect
*                   integrity of calling function.
*                      16-Aug-93   Pointers.
*   Inputs:
*       Name        Type        Length      Definition
*       x           t_float      1           Input to vector.
*       n           int         1           Size of vector.
*       v           t_float      n           Vector.
*   Outputs:
*       Name        Type        Length      Definition
*       *dx         t_float      1           Fraction of range for x.
*       *low        int         1           Current low end of range.
*       *high       int         1           Current high end of range.
*   Hardware dependencies:  ANSI C.
*   Header needed in scope of caller:   None.
*   Global variables used:  None.
*   Functions called:   None.
*/
void binsearch(t_float x, t_float *v, int n, int *high, int *low, t_float *dx)
{
  int mid;

  /* Initialize high and low  */
  *low = 0;
  *high = n - 1;

  /* Check endpoints  */
  if (x >= *(v + *high))
  {
    *low = *high;
    *dx = 0.;
  }
  else if (x <= *(v + *low))
  {
    *high = *low;
    *dx = 0.;
  }

  /* Search if necessary  */
  else
  {
    while ((*high - *low) > 1)
    {
      mid = (*low + *high) / 2;
      if (*(v + mid) > x)
        *high = mid;
      else
        *low = mid;
    }
    *dx = (x - *(v + *low)) / (*(v + *high) - *(v + *low));
  }
} /* End binsearch    */

/* T A B 1
*
*   Purpose:    Univariant arbitrarily spaced table look-up.
*
*   Author:     Dave Gutz 09-Jun-90.
*   Revisions:  Dave Gutz 20-Aug-90 Input x instead of *x to protect
*                   integrity of calling function.
*                         16-Aug-93   Pointers.
*   Inputs:
*       Name        Type        Length      Definition
*       n           int         1           Number of points.
*       x           t_float      1           Independent variable.
*       v           t_float      n           Breakpoint table.
*       y           t_float      n           Table data.
*   Outputs:
*       Name        Type        Length      Definition
*       tab1        t_float      1           Result of table lookup.
*   Hardware dependencies:  ANSI C.
*   Header needed in scope of caller:   None.
*   Global variables used:  None.
*   Functions called:   binsearch.
*/
t_float tab1(t_float x, t_float *v, t_float *y, int n)
{
  t_float dx;
  int high, low;
  void binsearch(t_float x, t_float *v, int n, int *high,
                 int *low, t_float *dx);
  if (n < 1)
    return y[0];
  binsearch(x, v, n, &high, &low, &dx);
  return *(y + low) + dx * (*(y + high) - *(y + low));
} /* End tab1 */

/* tab1clip:    Univariant arbitrarily spaced table look-up with clipping.
*
*   Author:     Dave Gutz 20-Nov-16
*   Inputs:
*       n           Number of points
*       x           Independent variable
*       v           Breakpoint table
*       y           Table data
*   Outputs:
*       tab1        Result of table lookup
*/
t_float tab1clip(t_float x, t_float *v, t_float *y, int n)
{
  t_float dx;
  int high, low;
  void binsearch(t_float x, t_float *v, int n, int *high,
                 int *low, t_float *dx);
  if (n < 1)
    return y[0];
  binsearch(x, v, n, &high, &low, &dx);
  return *(y + low) + fmax(fmin(dx, 1.), 0.) * (*(y + high) - *(y + low));
} /* End tab1clip */

/* T A B 2
*
*   Purpose:    Bivariant arbitrarily spaced table look-up.
*
*   Author:     Dave Gutz 20-Aug-90.
*   Revisions:            16-Aug-93   Pointers.
*   Inputs:
*       Name        Type        Length      Definition
*       n1          int         1           Number of ind var #1 brkpts.
*       n2          int         1           Number of ind var #2 brkpts.
*       x1          t_float      1           Independent variable #1.
*       x2          t_float      1           Independent variable #2.
*       v1          t_float      n1          Breakpoints for var #1.
*       v2          t_float      n2          Breakpoints for var #2.
*       y           t_float      n1*n2       Table data.
*   Outputs:
*       Name        Type        Length      Definition
*       tab2        t_float      1           Result of table lookup.
*   Hardware dependencies:  ANSI C.
*   Header needed in scope of caller:   None.
*   Global variables used:  None.
*   Functions called:   binsearch.
*/
t_float tab2(t_float x1, t_float x2, t_float *v1, t_float *v2, t_float *y, int n1,
            int n2)
{
  t_float dx1, dx2, r0, r1;
  int high1, high2, low1, low2, temp1, temp2;
  void binsearch(t_float x, t_float *v, int n, int *high,
                 int *low, t_float *dx);
  if (n1 < 1 || n2 < 1)
    return y[0];
  binsearch(x1, v1, n1, &high1, &low1, &dx1);
  binsearch(x2, v2, n2, &high2, &low2, &dx2);
  temp1 = low2 * n1 + low1;
  temp2 = high2 * n1 + low1;
  r0 = *(y + temp1) + dx1 * (*(y + low2 * n1 + high1) - *(y + temp1));
  r1 = *(y + temp2) + dx1 * (*(y + high2 * n1 + high1) - *(y + temp2));
  return r0 + dx2 * (r1 - r0);
} /* End tab2 */

// class TableInterp
// constructors
TableInterp::TableInterp()
    : n1_(0) {}
TableInterp::TableInterp(const unsigned int n, const t_float x[])
    : n1_(n)
{
  x_ = new t_float[n1_];
  for (unsigned int i = 0; i < n1_; i++)
  {
    x_[i] = x[i];
  }
}

TableInterp::~TableInterp() {}
// operators
// functions
t_float TableInterp::interp(void)
{
  return (-999.);
}
void TableInterp::pretty_print(void)
{
  Serial.printf("TableInterp:\n");
  Serial.printf(" x_ = {");
  for (unsigned int i = 0; i < n1_; i++)
  {
     Serial.printf("%7.3f, ", x_[i]);
  }
  Serial.printf("};\n");
  Serial.printf(" v_ = {");
  for (unsigned int i = 0; i < n1_; i++)
  {
     Serial.printf("%7.3f, ", v_[i]);
  }
  Serial.printf("};\n");
}

// 1-D Interpolation Table Lookup
// constructors
TableInterp1D::TableInterp1D() : TableInterp() {}
TableInterp1D::TableInterp1D(const unsigned int n, const t_float x[], const t_float v[])
    : TableInterp(n, x)
{
  v_ = new t_float[n1_];
  for (unsigned int i = 0; i < n1_; i++)
  {
    v_[i] = v[i];
  }
}
TableInterp1D::~TableInterp1D() {}
// operators
// functions
t_float TableInterp1D::interp(const t_float x)
{
  return (tab1(x, x_, v_, n1_));
}

// 1-D Interpolation Table Lookup
// constructors
TableInterp1Dclip::TableInterp1Dclip() : TableInterp() {}
TableInterp1Dclip::TableInterp1Dclip(const unsigned int n, const t_float x[], const t_float v[])
    : TableInterp(n, x)
{
  v_ = new t_float[n1_];
  for (unsigned int i = 0; i < n1_; i++)
  {
    v_[i] = v[i];
  }
}
TableInterp1Dclip::~TableInterp1Dclip() {}
// operators
// functions
t_float TableInterp1Dclip::interp(const t_float x)
{
  return (tab1(x, x_, v_, n1_));
}

// 2-D Interpolation Table Lookup
/* Example usage:  see voc_T_ in ../Battery.cpp.
x = {x1, x2, ...xn}
y = {y1, y2, ...ym}
v = {v11, v12, ...v1n, v21, v22, ...v2n, ...............  vm1, vm2, ...vmn}
  = {v11, v12, ...v1n,
     v21, v22, ...v2n,
     ...............
     vm1, vm2, ...vmn}
*/
// constructors
TableInterp2D::TableInterp2D() : TableInterp() {}
TableInterp2D::TableInterp2D(const unsigned int n, const unsigned int m, const t_float x[],
                             const t_float y[], const t_float v[])
    : TableInterp(n, x)
{
  n2_ = m;
  y_ = new t_float[n2_];
  for (unsigned int j = 0; j < n2_; j++)
  {
    y_[j] = y[j];
  }
  v_ = new t_float[n1_ * n2_];
  for (unsigned int i = 0; i < n1_; i++)
    for (unsigned int j = 0; j < n2_; j++)
    {
      v_[i + j * n1_] = v[i + j * n1_];
    }
}
TableInterp2D::~TableInterp2D() {}
// operators
// functions
t_float TableInterp2D::interp(t_float x, t_float y)
{
  return (tab2(x, y, x_, y_, v_, n1_, n2_));
}
//tab2(t_float x1, t_float x2, t_float *v1, t_float *v2, t_float *y, int n1, int n2);
/*
static t_float  xTbl[6]  =
  {-4.7, -1.88, -1.41, -.94, -.47, 4.7};
static t_float   yTbl[4]   =
  {0., 10000., 20000., 30000.};
static t_float  vTbl[24]  =
  {5.5, 3.5, 3.0, 2.5,
   5.5, 3.5, 3.0, 2.5,
   5.5, 3.5, 3.0, 5.5,
   5.5, 3.5, 5.5, 5.5,
   5.5, 5.5, 5.5, 5.5,
   5.5, 5.5, 5.5, 5.5};
    val = tab2(xDR_ven, fxven, xTbl, yTbl, vTbl,
    sizeof(xTbl)/sizeof(t_float),
    sizeof(yTbl)/sizeof(t_float)) * 100. / 4.7;
*/

