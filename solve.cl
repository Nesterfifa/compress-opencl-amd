/*******************************************************\
       0 1 2 3 4 5 6 7 8 9
     ┌──────── xtx ────────┐
   0 │ 0 . . . . . . . . . │
   1 │ 0 0 . . . . . . . . │
   2 │ 0 0 0 . . . . . . . │
   3 │ 0 0 0 0 . . . . . . │
   4 │ 0 0 0 0 0 . . . . . │
   5 │ 0 0 0 0 0 0 . . . . │  × total_jobs
   6 │ 0 0 0 0 0 0 0 . . . │
   7 │ 0 0 0 0 0 0 0 0 . . │
   8 │ 0 0 0 0 0 0 0 0 0 . │
   9 │ 0 0 0 0 0 0 0 0 0 0 │
     ├──────── xty ────────┤
  10 │ 0 0 0 0 0 0 0 0 0 0 │
     └─────────────────────┘
\*******************************************************/

#ifndef JOBS_PER_GROUP
#define JOBS_PER_GROUP 5
#endif

int gaussian_elimination(
  __local double matrix[10][10 + 1],
  __local double column[10],
  int start, int end, int job, int thread, bool active, int max_k
);

__attribute__((vec_type_hint(double)))
__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void solve(
  __global const double *xtx_and_xty, int total_jobs,
  __global double *w5, __global double *w6, __global double *w10
) {
  const int local_id = get_local_id(0);      // in [0, 64)
  const int group_id = get_group_id(0);      // in [0, ⌈H/5⌉)

  const int jobs = min(JOBS_PER_GROUP, total_jobs - group_id * JOBS_PER_GROUP);

  __local double matrices[JOBS_PER_GROUP][10][10 + 1];

  for (int offset = 0; offset < jobs * (55 + 10); offset += 64) {
    int job = (offset + local_id) / (55 + 10);
    int idx = (offset + local_id) % (55 + 10);
    int col = (int) floor(sqrt(0.25 + 2 * idx) - 0.5);
    int row = (int) (idx - col * (col + 1) / 2);
    if (offset + local_id < jobs * (55 + 10)) {
      matrices[job][row][col] = xtx_and_xty[group_id * JOBS_PER_GROUP * (55 + 10) + offset + local_id];
      if (idx < 55) matrices[job][col][row] = matrices[job][row][col];
    }
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  __local double columns[JOBS_PER_GROUP][10];
  int job = local_id / (10 + 1);
  int thread = local_id % (10 + 1);
  bool active = job < jobs;

  // printf("Group %3d, local_id %2d: job %d/%d, thread %2d [%d]\n", group_id, local_id, job, jobs, thread, active);

  int max_k = gaussian_elimination(matrices[job], columns[job], 0, 5, job, thread, active, 0);

  if (active && thread < 5) {
    double value = max_k >= 5 ? nan(0u) : matrices[job][thread][10];
    w5[group_id * JOBS_PER_GROUP * 5 + job * 5 + thread] = value;
  }

  max_k = gaussian_elimination(matrices[job], columns[job], 5, 6, job, thread, active, max_k);

  if (active && thread < 6) {
    double value = max_k >= 6 ? nan(0u) : matrices[job][thread][10];
    w6[group_id * JOBS_PER_GROUP * 6 + job * 6 + thread] = value;
  }

  max_k = gaussian_elimination(matrices[job], columns[job], 6, 10, job, thread, active, max_k);

  if (active && thread < 10) {
    double value = max_k >= 10 ? nan(0u) : matrices[job][thread][10];
    w10[group_id * JOBS_PER_GROUP * 10 + job * 10 + thread] = value;
  }
}

int gaussian_elimination(
  __local double matrix[10][10 + 1],
  __local double column[10],
  int start, int end, int job, int thread, bool active, int max_k
) {
  for (int j = start; j < end; ++j) {
    int k = j;

    if (active) {
      if (thread < 10) column[thread] = matrix[thread][j];

      if (fabs(matrix[j][j]) < 0.0000001) {
        for (int i = k + 1; i < end; ++i) {
          if (fabs(matrix[i][j]) > fabs(matrix[k][j])) k = i;
        }
        if (fabs(matrix[k][j]) < 0.0000000001) {
          for (int i = end; i < 10; ++i) {
            if (fabs(matrix[i][j]) > fabs(matrix[k][j])) k = i;
          }
        }
        if (max_k < k) max_k = k;
        if (k != j) {
          if (thread == j) column[thread] = matrix[k][j];
          if (thread == k) column[thread] = matrix[j][j];
        }
      }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (active && max_k < 10) {
      if (k != j) {
//        matrix[j][thread] += matrix[k][thread];
        double value = matrix[j][thread];
        matrix[j][thread] = matrix[k][thread];
        matrix[k][thread] = value;
      }
      matrix[j][thread] /= column[j];
      for (int i = 0; i < 10; ++i) {
        if (i != j) matrix[i][thread] -= matrix[j][thread] * column[i];
      }
    }

    barrier(CLK_LOCAL_MEM_FENCE);
  }

  return max_k;
}
