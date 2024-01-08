#include "ratings_cuda.h"
#include "timer.h"

#include <algorithm>

__global__ void ratings_error(int players, double* ratings, double* scores, double* error,
int* indices, int* opp_played, int* opp_index)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= players)
  {
    return;
  }

  double score = 0;
  for (int k = indices[i]; k != indices[i+1]; ++k)
  {
    score += opp_played[k] * ratings[i] / (ratings[i] + ratings[opp_index[k]]);
  }
  error[i] = scores[i] - score;
}

__global__ void adjust_ratings(int players, double K, double* ratings, 
  double* error, int* played)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= players)
  {
    return;
  }

  ratings[i] = ratings[i] * pow(10.0, K * error[i] / played[i]);
}

template <typename T>
void cuda_copy(const std::vector<T>& v, T*& dest)
{
  cudaMalloc(&dest, v.size() * sizeof(T)); 
  cudaMemcpy(dest, v.data(), v.size()*sizeof(T), cudaMemcpyHostToDevice);
}

void RatingsCuda::find_ratings()
{
  cuda_copy(ratings_, d_ratings);
  cuda_copy(scores, d_scores);
  cuda_copy(opp_played, d_opp_played);
  cuda_copy(opp_index, d_opp_index);
  cuda_copy(indices_, d_indices);
  cuda_copy(played_, d_played);

  cudaMalloc(&d_errors, players_*sizeof(double));

  const int block_size = 128;

  for (int i = 0; i != 3000; ++i)
  {
    ratings_error<<<(players_+1)/block_size, block_size>>>(players_, d_ratings, d_scores,
      d_errors, d_indices, d_opp_played, d_opp_index);

    adjust_ratings<<<players_+1/block_size, block_size>>>(players_, 1.6, d_ratings, d_errors,
    d_played);
  }

  cudaMemcpy(ratings_.data(), d_ratings, players_ * sizeof(double),
    cudaMemcpyDeviceToHost);
  cudaMemcpy(errors_.data(), d_errors, players_ * sizeof(double),
    cudaMemcpyDeviceToHost);

  cudaFree(d_ratings);
  cudaFree(d_scores);
  cudaFree(d_errors);
  cudaFree(d_indices);
  cudaFree(d_opp_played);
  cudaFree(d_opp_index);
  cudaFree(d_played);
}
