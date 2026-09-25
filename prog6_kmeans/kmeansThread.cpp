#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>

#include "CycleTimer.h"

using namespace std;

typedef struct {
  // Control work assignments
  int start, end;

  // Shared by all functions
  double *data;
  double *clusterCentroids;
  int *clusterAssignments;
  double *currCost;
  int M, N, K;
} WorkerArgs;


/**
 * Checks if the algorithm has converged.
 * 
 * @param prevCost Pointer to the K dimensional array containing cluster costs 
 *    from the previous iteration.
 * @param currCost Pointer to the K dimensional array containing cluster costs 
 *    from the current iteration.
 * @param epsilon Predefined hyperparameter which is used to determine when
 *    the algorithm has converged.
 * @param K The number of clusters.
 * 
 * NOTE: DO NOT MODIFY THIS FUNCTION!!!
 */
static bool stoppingConditionMet(double *prevCost, double *currCost,
                                 double epsilon, int K) {
  for (int k = 0; k < K; k++) {
    if (abs(prevCost[k] - currCost[k]) > epsilon)
      return false;
  }
  return true;
}

/**
 * Computes L2 distance between two points of dimension nDim.
 * 
 * @param x Pointer to the beginning of the array representing the first
 *     data point.
 * @param y Poitner to the beginning of the array representing the second
 *     data point.
 * @param nDim The dimensionality (number of elements) in each data point
 *     (must be the same for x and y).
 */
double dist(double *x, double *y, int nDim) {
  double accum = 0.0;
  for (int i = 0; i < nDim; i++) {
    accum += pow((x[i] - y[i]), 2);
  }
  return sqrt(accum);
}

/**
 * Assigns a half-open range of data points to their closest centroids.
 * Each point is written exactly once, so separate ranges can run concurrently.
 */
static void computeAssignmentRange(WorkerArgs *const args) {
  for (int m = args->start; m < args->end; m++) {
    double minDist = 1e30;
    int bestAssignment = -1;

    for (int k = 0; k < args->K; k++) {
      double d = dist(&args->data[m * args->N],
                      &args->clusterCentroids[k * args->N], args->N);
      if (d < minDist) {
        minDist = d;
        bestAssignment = k;
      }
    }

    args->clusterAssignments[m] = bestAssignment;
  }
}

/**
 * Assigns each data point to its "closest" cluster centroid in parallel.
 */
void computeAssignments(WorkerArgs *const args) {
  const int rangeStart = max(0, args->start);
  const int rangeEnd = min(args->M, args->end);
  const int workItems = rangeEnd - rangeStart;
  if (workItems <= 0) {
    return;
  }

  unsigned int availableWorkers = thread::hardware_concurrency();
  int workerCount = availableWorkers == 0
                        ? 1
                        : min(static_cast<unsigned int>(workItems),
                              availableWorkers);
  if (workerCount == 1) {
    WorkerArgs worker = *args;
    worker.start = rangeStart;
    worker.end = rangeEnd;
    computeAssignmentRange(&worker);
    return;
  }

  const int chunkSize = (workItems + workerCount - 1) / workerCount;
  vector<WorkerArgs> workers(workerCount);
  vector<thread> threads;
  threads.reserve(workerCount - 1);

  for (int workerId = 0; workerId < workerCount; workerId++) {
    workers[workerId] = *args;
    workers[workerId].start = rangeStart + workerId * chunkSize;
    workers[workerId].end = min(rangeEnd, workers[workerId].start + chunkSize);
  }

  // The calling thread performs the first range to avoid an extra worker.
  for (int workerId = 1; workerId < workerCount; workerId++) {
    threads.emplace_back(computeAssignmentRange, &workers[workerId]);
  }
  computeAssignmentRange(&workers[0]);
  for (thread &worker : threads) {
    worker.join();
  }
}

/**
 * Given the cluster assignments, computes the new centroid locations for
 * each cluster.
 */
void computeCentroids(WorkerArgs *const args) {
  int *counts = new int[args->K];

  // Zero things out
  for (int k = 0; k < args->K; k++) {
    counts[k] = 0;
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] = 0.0;
    }
  }


  // Sum up contributions from assigned examples
  for (int m = 0; m < args->M; m++) {
    int k = args->clusterAssignments[m];
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] +=
          args->data[m * args->N + n];
    }
    counts[k]++;
  }

  // Compute means
  for (int k = 0; k < args->K; k++) {
    counts[k] = max(counts[k], 1); // prevent divide by 0
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] /= counts[k];
    }
  }

  delete[] counts;
}

/**
 * Computes the per-cluster cost. Used to check if the algorithm has converged.
 */
void computeCost(WorkerArgs *const args) {
  double *accum = new double[args->K];

  // Zero things out
  for (int k = 0; k < args->K; k++) {
    accum[k] = 0.0;
  }

  // Sum cost for all data points assigned to centroid
  for (int m = 0; m < args->M; m++) {
    int k = args->clusterAssignments[m];
    accum[k] += dist(&args->data[m * args->N],
                     &args->clusterCentroids[k * args->N], args->N);
  }

  // Update costs
  for (int k = args->start; k < args->end; k++) {
    args->currCost[k] = accum[k];
  }

  delete[] accum;
}

/**
 * Computes the K-Means algorithm, using std::thread to parallelize the work.
 *
 * @param data Pointer to an array of length M*N representing the M different N 
 *     dimensional data points clustered. The data is layed out in a "data point
 *     major" format, so that data[i*N] is the start of the i'th data point in 
 *     the array. The N values of the i'th datapoint are the N values in the 
 *     range data[i*N] to data[(i+1) * N].
 * @param clusterCentroids Pointer to an array of length K*N representing the K 
 *     different N dimensional cluster centroids. The data is laid out in
 *     the same way as explained above for data.
 * @param clusterAssignments Pointer to an array of length M representing the
 *     cluster assignments of each data point, where clusterAssignments[i] = j
 *     indicates that data point i is closest to cluster centroid j.
 * @param M The number of data points to cluster.
 * @param N The dimensionality of the data points.
 * @param K The number of cluster centroids.
 * @param epsilon The algorithm is said to have converged when
 *     |currCost[i] - prevCost[i]| < epsilon for all i where i = 0, 1, ..., K-1
 */
void kMeansThread(double *data, double *clusterCentroids, int *clusterAssignments,
               int M, int N, int K, double epsilon) {

  // Used to track convergence
  double *prevCost = new double[K];
  double *currCost = new double[K];
  double assignmentSeconds = 0.0;
  double centroidSeconds = 0.0;
  double costSeconds = 0.0;
  double totalStartTime = CycleTimer::currentSeconds();

  // The WorkerArgs array is used to pass inputs to and return output from
  // functions.
  WorkerArgs args;
  args.data = data;
  args.clusterCentroids = clusterCentroids;
  args.clusterAssignments = clusterAssignments;
  args.currCost = currCost;
  args.M = M;
  args.N = N;
  args.K = K;

  // Initialize arrays to track cost
  for (int k = 0; k < K; k++) {
    prevCost[k] = 1e30;
    currCost[k] = 0.0;
  }

  /* Main K-Means Algorithm Loop */
  int iter = 0;
  while (!stoppingConditionMet(prevCost, currCost, epsilon, K)) {
    // Update cost arrays (for checking convergence criteria)
    for (int k = 0; k < K; k++) {
      prevCost[k] = currCost[k];
    }

    // Setup args struct for the point-range assignment workers.
    args.start = 0;
    args.end = M;

    double stageStartTime = CycleTimer::currentSeconds();
    computeAssignments(&args);
    assignmentSeconds += CycleTimer::currentSeconds() - stageStartTime;

    stageStartTime = CycleTimer::currentSeconds();
    computeCentroids(&args);
    centroidSeconds += CycleTimer::currentSeconds() - stageStartTime;

    // Cost writes one independent slot per cluster, so use a cluster range.
    args.start = 0;
    args.end = K;
    stageStartTime = CycleTimer::currentSeconds();
    computeCost(&args);
    costSeconds += CycleTimer::currentSeconds() - stageStartTime;

    iter++;
  }

  double totalSeconds = CycleTimer::currentSeconds() - totalStartTime;
  printf("[Profile]: iterations=%d assignment=%.3f ms centroid=%.3f ms "
         "cost=%.3f ms total=%.3f ms\n",
         iter, assignmentSeconds * 1000, centroidSeconds * 1000,
         costSeconds * 1000, totalSeconds * 1000);

  delete[] currCost;
  delete[] prevCost;
}
