#pragma once

inline int i4vec_sum( int n, int* a )
{
  int i;
  int sum;

  sum = 0;
  for ( i = 0; i < n; i++ )
    sum = sum + a[ i ];

  return sum;
}

inline float* halton_base( int i, int m, const int* b )

/******************************************************************************
  Parameters:

    Input, int I, the index of the element of the sequence.
    0 <= I.

    Input, int M, the spatial dimension.

    Input, int B[M], the bases to use for each dimension.

    Output, float HALTON_BASE[M], the element of the sequence with index I.
*/
{
  static float b_inv[10];
  static float r[10];
  static int   t[10];

  int d;
  int j;

  for ( j = 0; j < m; j++ )
    t[ j ] = i;

  for ( j = 0; j < m; j++ )
    b_inv[ j ] = 1.0f / b[ j ];

  for ( j = 0; j < m; j++ )
    r[ j ] = 0.0f;

  while ( 0 < i4vec_sum( m, t ) )
  {
    for ( j = 0; j < m; j++ )
    {
      d = ( t[ j ] % b[ j ] );
      r[ j ] = r[ j ] + (float)(d)*b_inv[ j ];
      b_inv[ j ] = b_inv[ j ] / (float)( b[ j ] );
      t[ j ] = ( t[ j ] / b[ j ] );
    }
  }

  return r;
}