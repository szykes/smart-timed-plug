#ifndef TEST_MOCK_H_
#define TEST_MOCK_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TYPE_NONE = 0,
  TYPE_UINT8_T,
  TYPE_UINT16_T,
  TYPE_BUTTON_E,
} type_e;

typedef struct {
  type_e type;
  void *value;
  size_t size;
} type_st;

typedef struct mock_call_st {
  char function_name[50];
  type_st params[2];
  type_st ret;

  bool is_called;
  char result[110];
  bool is_matched;
  char place[50];
  char message[100];

  struct mock_call_st *next;
} mock_call_st;

extern mock_call_st *mock_calls_head;

#define mock_prepare_param(dest, data)		\
  dest = (void*)malloc(sizeof(data));		\
  memcpy(dest, &data, sizeof(data));

void mock_init(void);
void mock_clear_calls(void);

#define mock_initiate_expectation(function_name, params, no_params, ret) \
  __mock_initiate_expectation(function_name, params, no_params, ret, __func__, __LINE__, "")
#define mock_initiate_expectation_with_msg(function_name, params, no_params, ret, fmt, ...) \
  __mock_initiate_expectation(function_name, params, no_params, ret, __func__, __LINE__, fmt, ##__VA_ARGS__)
void __mock_initiate_expectation(const char *function_name, type_st *params, size_t no_params, type_st *ret, const char *func, unsigned int line, const char *fmt, ...);

#define mock_record(params, no_params, ret) __mock_record(__func__, params, no_params, ret)
void __mock_record(const char *function_name, type_st *params, size_t no_params, type_st *ret);

bool mock_is_succeeded(void);

#endif // TEST_MOCK_H_
