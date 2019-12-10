/* This file is part of ESDM.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with ESDM.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This test uses the ESDM high-level API to actually write a contiuous ND subset of a data set
 */


#include <esdm.h>
#include <esdm-internal.h>
#include <test/util/test_util.h>

#include <stdio.h>
#include <stdlib.h>

int64_t totalFragmentCount = 0, totalWrittenData = 0;
double totalWriteTime = 0, totalReadTime = 0;

void initData(int64_t length, uint8_t* data) {
  for(int i = 0; i < length; i++) {
    data[i] = i%256;
  }
}

bool dataIsCorrect(int64_t length, uint8_t* data) {
  for(int i = 0; i < length; i++) {
    if(data[i] != i%256) return false;
  }
  return true;
}

//writes all possible fragments
void writeData(esdm_dataset_t* dataset, esdm_dataspace_t* dataspace, int64_t length, uint8_t *data) {
  int ret;
  int64_t fragmentCount = 0;
  int64_t totalSize = 0;
  timer myTimer;
  start_timer(&myTimer);

  for(int64_t offset = 0; offset < length; offset++) {
    for(int64_t size = 1; offset + size <= length; size++) {
      esdm_statistics_t beforeStats = esdm_write_stats();

      esdm_dataspace_t *subspace;
      ret = esdm_dataspace_subspace(dataspace, 1, &size, &offset, &subspace);
      eassert(ret == ESDM_SUCCESS);

      ret = esdm_write(dataset, &data[offset], subspace);
      eassert(ret == ESDM_SUCCESS);

      ret = esdm_dataspace_destroy(subspace);
      eassert(ret == ESDM_SUCCESS);

      fragmentCount++;
      totalSize += size * sizeof(*data);

      //check statistics
      esdm_statistics_t afterStats = esdm_write_stats();
      eassert(afterStats.bytesUser - beforeStats.bytesUser == size * sizeof(*data));
      eassert(afterStats.bytesIo - beforeStats.bytesIo == size * sizeof(*data));
      eassert(afterStats.requests - beforeStats.requests == 1);
      eassert(afterStats.fragments - beforeStats.fragments == 1);
    }
  }

  ret = esdm_dataset_commit(dataset);
  eassert(ret == ESDM_SUCCESS);

  double writeTime = stop_timer(myTimer);
  printf("write %"PRId64" fragments of %"PRId64" bytes of data (%"PRId64" bytes total): %.3fms\n", fragmentCount, length*sizeof(*data), totalSize, 1000*writeTime);
  totalFragmentCount += fragmentCount;
  totalWrittenData += totalSize;
  totalWriteTime += writeTime;
}

void readRandomFragment(esdm_dataset_t* dataset, esdm_dataspace_t* dataspace, int64_t length, uint8_t* data) {
  //XXX: If we would select x0 randomly first, and then select x1 from the remaining eligible range, we would not need the trial-and-error loop.
  //     However, we would pay with a non-uniform distribution of the intervals:
  //     x0 would be trivially equally distributed, but x1 would have a heavy bias towards the end of the data array.
  //     To be precise, the interval [length-1, length-1] would have a probability of 1/length, but the interval [0, 0] would have a probability of 1/length^2.
  //
  //     The trial-and-error loop exists to avoid this uneven distribution of intervals,
  //     ensuring that every possible interval is selected exactly with a probability of 2/(length*(length + 1)).
  int64_t x0, x1;
  do {
    x0 = length*random()/((int64_t)RAND_MAX + 1);  //[0, length - 1]
    x1 = length*random()/((int64_t)RAND_MAX + 1) + 1;  //[1, length]
  } while(x0 >= x1);
  int64_t fragmentSize = x1 - x0;

  esdm_statistics_t beforeStatsRead = esdm_read_stats();

  esdm_dataspace_t* subspace;
  int ret = esdm_dataspace_subspace(dataspace, 1, &fragmentSize, &x0, &subspace);
  eassert(ret == ESDM_SUCCESS);
  ret = esdm_dataspace_copyDatalayout(subspace, dataspace);
  eassert(ret == ESDM_SUCCESS);

  ret = esdm_read(dataset, &data[x0], subspace);
  eassert(ret == ESDM_SUCCESS);

  ret = esdm_dataspace_destroy(subspace);
  eassert(ret == ESDM_SUCCESS);

  //check statistics
  esdm_statistics_t afterStatsRead = esdm_read_stats();

  eassert(afterStatsRead.bytesUser - beforeStatsRead.bytesUser == fragmentSize*sizeof(*data));
  eassert(afterStatsRead.bytesInternal - beforeStatsRead.bytesInternal == 0);
  eassert(afterStatsRead.bytesIo - beforeStatsRead.bytesIo == fragmentSize*sizeof(*data));
  eassert(afterStatsRead.requests - beforeStatsRead.requests == 1);
  eassert(afterStatsRead.internalRequests - beforeStatsRead.internalRequests == 0);
  eassert(afterStatsRead.fragments - beforeStatsRead.fragments == 1);
}

void printUsage(const char* programPath) {
  printf("Usage:\n");
  printf("%s [(-l|--length) L] [(-c|--count) C] [-?|-h|--help]\n", programPath);
  printf("\n");
  printf("\t-l L, --length L\n");
  printf("\t\tSet the length of the dataset to write.\n");
  printf("\n");
  printf("\t\tNote that the fragment count is quadratic to the length,\n");
  printf("\t\tso the entire benchmark has an O(L^3) complexity in time and space.");
  printf("\n");
  printf("\t-c C, --count C\n");
  printf("\t\tSet the count of fragments that are read back from the dataset.\n");
  printf("\n");
  printf("\t-r R, --repetitions R\n");
  printf("\t\tNumber of times that the entire test is to be repeated,\n");
  printf("\t\tincluding reinitialization and recreation of the ESDM file system.\n");
  printf("\n");
  printf("\t-?, -h, --help\n");
  printf("\t\tPrint this usage information and exit.\n");
}

//argv[0] is expected to be the option name, argv[1] the integer that we need to parse
int64_t readIntArg(int argc, char const **argv) {
  if(argc < 2) {
    fprintf(stderr, "error: %s option needs an integer argument\n", *argv);
    exit(1);
  }
  char* endPtr;
  int64_t result = strtol(argv[1], &endPtr, 0);
  if(!*argv[1] || *endPtr) {
    fprintf(stderr, "error: the argument \"%s\" to the %s option is not an integer\n", argv[1], argv[0]);
    exit(1);
  }
  return result;
}

void readArgs(int argc, char const **argv, int64_t* out_length, int64_t* out_readCount, int64_t* out_repetitions) {
  //save the program name
  eassert(argc > 0);
  char const* programPath = *argv++;
  argc--;

  //defaults
  *out_length = 70;  //2485 fragments
  *out_readCount = 100;
  *out_repetitions = 1;

  for(; argc > 0; argc--, argv++) {
    if(!strcmp(*argv, "-l") || !strcmp(*argv, "--length")) {
      *out_length = readIntArg(argc--, argv++); //gobble up an additional argument;
    } else if(!strcmp(*argv, "-c") || !strcmp(*argv, "--count")) {
      *out_readCount = readIntArg(argc--, argv++);  //gobble up an additional argument;
    } else if(!strcmp(*argv, "-r") || !strcmp(*argv, "--repetitions")) {
      *out_repetitions = readIntArg(argc--, argv++);  //gobble up an additional argument;
    } else if(!strcmp(*argv, "-?") || !strcmp(*argv, "-h") || !strcmp(*argv, "--help")) {
      printUsage(programPath);
      exit(0);
    } else {
      fprintf(stderr, "error: unrecognized option \"%s\"\n\n", *argv);
      printUsage(programPath);
      exit(1);
    }
  }
}

//Benchmark idea:
//
//Write a 1D dataset with *all* possible fragments.
//I.e. a 1k dataset may contain up to 500500 different fragments with 167.167 mega-entries total, so each fragment contains 334 entries on average.
//This allows us to stress test the handling of many small fragments, without causing significant I/O times in the read path.
//
//XXX: This benchmark is written to use 1D data because 1D data provides the highest possible fragment count per given stored data volume.
void runTestWithConfig(int64_t length, int64_t readCount, const char* configString) {
  esdm_status ret = esdm_load_config_str(configString);
  eassert(ret == ESDM_SUCCESS);
  ret = esdm_init();
  eassert(ret == ESDM_SUCCESS);

  esdm_loglevel(ESDM_LOGLEVEL_WARNING); //stop the esdm_mkfs() call from spamming us with infos about deleted objects
  ret = esdm_mkfs(ESDM_FORMAT_PURGE_RECREATE, ESDM_ACCESSIBILITY_GLOBAL);
  eassert(ret == ESDM_SUCCESS);
  ret = esdm_mkfs(ESDM_FORMAT_PURGE_RECREATE, ESDM_ACCESSIBILITY_NODELOCAL);
  eassert(ret == ESDM_SUCCESS);
  esdm_loglevel(ESDM_LOGLEVEL_INFO);

  // define dataspace
  esdm_dataspace_t *dataspace;
  ret = esdm_dataspace_create(1, &length, SMD_DTYPE_UINT8, &dataspace);
  eassert(ret == ESDM_SUCCESS);
  esdm_container_t *container;
  ret = esdm_container_create("mycontainer", 1, &container);
  eassert(ret == ESDM_SUCCESS);

  esdm_dataset_t *dataset;
  ret = esdm_dataset_create(container, "dataset", dataspace, &dataset);
  eassert(ret == ESDM_SUCCESS);
  ret = esdm_dataset_commit(dataset);
  eassert(ret == ESDM_SUCCESS);
  ret = esdm_container_commit(container);
  eassert(ret == ESDM_SUCCESS);

  // perform the test
  uint8_t* data = malloc(length*sizeof(*data));
  initData(length, data);

  writeData(dataset, dataspace, length, data);

  timer myTimer;
  start_timer(&myTimer);
  for(int64_t i = 0; i < readCount; i++) readRandomFragment(dataset, dataspace, length, data);
  double readTime = stop_timer(myTimer);
  printf("read %"PRId64" random fragments: %.3fms\n", readCount, 1000*readTime);
  totalReadTime += readTime;
  eassert(dataIsCorrect(length, data));

  ret = esdm_finalize();
  eassert(ret == ESDM_SUCCESS);
}

int main(int argc, char const *argv[]) {
  int64_t length, readCount, repetitions;
  readArgs(argc, argv, &length, &readCount, &repetitions);

  double arrayReadTime = 0, btreeReadTime = 0;
  for(int64_t i = 0; i < repetitions; i++) {
    totalReadTime = 0;
    printf("\n\n=== array based bound list ===\n\n");
    runTestWithConfig(length, readCount, "{ \"esdm\": { \"bound list implementation\": \"array\", \"backends\": [ { \"type\": \"POSIX\", \"id\": \"p1\", \"accessibility\": \"global\", \"target\": \"./_posix1\" } ], \"metadata\": { \"type\": \"metadummy\", \"id\": \"md\", \"target\": \"./_metadummy\" } } }");
    arrayReadTime += totalReadTime;

    totalReadTime = 0;
    printf("\n\n=== B-tree based bound list ===\n\n");
    runTestWithConfig(length, readCount, "{ \"esdm\": { \"bound list implementation\": \"btree\", \"backends\": [ { \"type\": \"POSIX\", \"id\": \"p1\", \"accessibility\": \"global\", \"target\": \"./_posix1\" } ], \"metadata\": { \"type\": \"metadummy\", \"id\": \"md\", \"target\": \"./_metadummy\" } } }");
    btreeReadTime += totalReadTime;
  }

  printf("\nTotals:\n");
  printf("\twritten data: %"PRId64" bytes in %"PRId64" fragments (%g bytes/fragment avg)\n", totalWrittenData, totalFragmentCount, totalWrittenData/(double)totalFragmentCount);
  printf("\twrite time: %.3fs (%.3fs avg)\n", totalWriteTime, totalWriteTime/2/repetitions);
  printf("\tread time (array based bound list): %.3fs (%.3fs avg, %.3fms per call, %.3fus per considered fragment)\n", arrayReadTime, arrayReadTime/repetitions, arrayReadTime*1000/repetitions/readCount, arrayReadTime*1000*1000/readCount/totalFragmentCount*2);
  printf("\tread time (B-tree based bound list): %.3fs (%.3fs avg, %.3fms per call, %.3fus per considered fragment)\n", btreeReadTime, btreeReadTime/repetitions, btreeReadTime*1000/repetitions/readCount, btreeReadTime*1000*1000/readCount/totalFragmentCount*2);

  printf("\nOK\n");

  return 0;
}
