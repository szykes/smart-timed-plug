#ifndef TEST_MOCK_H_
#define TEST_MOCK_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef int mock_skip;
#define MOCK_SKIP_T "mock_skip"

typedef struct {
  char *type;
  void *value;
  size_t size;
} type_st;

typedef struct mock_call_st {
  char function_name[50];
  type_st params[2];
  type_st ret;

  bool is_called;
  char result[150];
  bool is_matched;
  bool is_expected;
  char place[50];
  char message[100];

  struct mock_call_st *next;
} mock_call_st;

inline static char *strdup(const char *s) {
  size_t size = strlen(s) + 1;
  char *p = malloc(size);
  if (p != NULL) {
    memcpy(p, s, size);
  }
  return p;
}

#define mock_prepare_param(dest, data)		\
  dest = (void*)malloc(sizeof(data));		\
  memcpy(dest, &data, sizeof(data));

void mock_init(void);

void mock_uninterested_call(const char *function_name);
void mock_clear_all_uninterested_calls(void);

void mock_clear_calls(void);

// expect
#define MOCK_EXPECT(function_name, fmt, ...)			\
  __mock_initiate_expectation(function_name, NULL, 0, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__);

#define MOCK_EXPECT_RET(function_name, ret_type, ret_value, fmt, ...)	\
  {									\
    void *ret_ptr;							\
    mock_prepare_param(ret_ptr, ret_value);				\
    type_st ret = {							\
      .type = strdup(#ret_type),					\
      .value = ret_ptr,							\
      .size = sizeof(ret_value),					\
    };									\
    __mock_initiate_expectation(function_name, NULL, 0, &ret, __func__, __LINE__, fmt, ##__VA_ARGS__); \
  } while(0);

#define MOCK_EXPECT_1_PARAM(function_name, param_1_type, param_1_value, fmt, ...) \
  {									\
    void *param_1_ptr;							\
    mock_prepare_param(param_1_ptr, param_1_value);			\
    type_st params[] = {						\
      {									\
	 .type = strdup(#param_1_type),					\
         .value = param_1_ptr,						\
         .size = sizeof(param_1_value),		       			\
      }									\
    };									\
    __mock_initiate_expectation(function_name, params, sizeof(params) / sizeof(type_st), NULL, __func__, __LINE__, fmt, ##__VA_ARGS__); \
  } while(0);

#define MOCK_EXPECT_1_PARAM_RET(function_name, param_1_type, param_1_value, ret_type, ret_value, fmt, ...) \
  {									\
    void *ret_ptr;							\
    mock_prepare_param(ret_ptr, ret_value);                             \
    void *param_1_ptr;                                                  \
    mock_prepare_param(param_1_ptr, param_1_value);                     \
    type_st params[] = {						\
      {									\
	 .type = strdup(#param_1_type),					\
         .value = param_1_ptr,						\
         .size = sizeof(param_1_value),		       			\
      }									\
    };									\
    type_st ret = {                                                     \
      .type = strdup(#ret_type),					\
      .value = ret_ptr,                                                 \
      .size = sizeof(ret_value),                                        \
    };                                                                  \
    __mock_initiate_expectation(function_name, params, sizeof(params) / sizeof(type_st), &ret, __func__, __LINE__, fmt, ##__VA_ARGS__); \
  } while(0);

#define MOCK_EXPECT_2_PARAM(function_name, param_1_type, param_1_value, param_2_type, param_2_value, fmt, ...) \
  {									\
    void *param_1_ptr;                                                  \
    mock_prepare_param(param_1_ptr, param_1_value);                     \
    void *param_2_ptr;                                                  \
    mock_prepare_param(param_2_ptr, param_2_value);                     \
    type_st params[] = {						\
      {									\
	 .type = strdup(#param_1_type),					\
         .value = param_1_ptr,						\
         .size = sizeof(param_1_value),		       			\
      },								\
      {									\
	 .type = strdup(#param_2_type),					\
         .value = param_2_ptr,						\
         .size = sizeof(param_2_value),		       			\
      },								\
    };									\
    __mock_initiate_expectation(function_name, params, sizeof(params) / sizeof(type_st), NULL, __func__, __LINE__, fmt, ##__VA_ARGS__); \
  } while(0);

#define MOCK_EXPECT_2_PARAM_RET(function_name, param_1_type, param_1_value, param_2_type, param_2_value, ret_type, ret_value, fmt, ...)) \
  {									\
    void *ret_ptr;                                                      \
    mock_prepare_param(ret_ptr, ret_value);                             \
    void *param_1_ptr;                                                  \
    mock_prepare_param(param_1_ptr, param_1_value);                     \
    void *param_2_ptr;                                                  \
    mock_prepare_param(param_2_ptr, param_2_value);                     \
    type_st params[] = {                                                \
      {                                                                 \
	 .type = strdup(#param_1_type),					\
         .value = param_1_ptr,                                          \
         .size = sizeof(param_1_value),                                 \
      },                                                                \
      {                                                                 \
	 .type = strdup(#param_2_type),					\
         .value = param_2_ptr,                                          \
         .size = sizeof(param_2_value),                                 \
      },                                                                \
    };                                                                  \
    type_st ret = {                                                     \
      .type = strdup(#ret_type),					\
      .value = ret_ptr,                                                 \
      .size = sizeof(ret_value),                                        \
    };                                                                  \
    __mock_initiate_expectation(function_name, params, sizeof(params) / sizeof(type_st), &ret, __func__, __LINE__, fmt, ##__VA_ARGS__); \
  } while(0);

void __mock_initiate_expectation(const char *function_name, type_st *params, size_t no_params, type_st *ret, const char *func, unsigned int line, const char *fmt, ...);

// record
#define MOCK_RECORD() \
  __mock_record(__func__, NULL, 0, MOCK_SKIP_T, NULL);

#define MOCK_RECORD_RET(ret_type)					\
  {									\
    type_st ret;							\
    ret.type = strdup(#ret_type);					\
    __mock_record(__func__, NULL, 0, #ret_type, &ret);			\
    if (strcmp(ret.type, #ret_type) == 0) {				\
      return *((ret_type*)ret.value);					\
    }									\
    return (ret_type)0;                                                 \
  } while(0);

#define MOCK_RECORD_1_PARAM(param_1_type, param_1_value)		\
  {									\
    type_st params[] = {						\
      {									\
	 .type = strdup(#param_1_type),					\
         .value = &param_1_value,		       			\
         .size = sizeof(param_1_value),		       			\
      }									\
    };									\
    __mock_record(__func__, params, sizeof(params) / sizeof(type_st), MOCK_SKIP_T, NULL); \
  } while(0);

#define MOCK_RECORD_1_PARAM_RET(param_1_type, param_1_value, ret_type)	\
  {									\
    type_st ret;							\
    ret.type = strdup(#ret_type);					\
    type_st params[] = {						\
      {									\
	 .type = strdup(#param_1_type),					\
         .value = &param_1_value,		       			\
         .size = sizeof(param_1_value),		       			\
      }									\
    };									\
    __mock_record(__func__, params, sizeof(params) / sizeof(type_st), #ret_type, &ret); \
    if (strcmp(ret.type, #ret_type) == 0) {				\
      return *((ret_type*)ret.value);                                   \
    }									\
    return (ret_type)0;                                                 \
  } while(0);

#define MOCK_RECORD_2_PARAM(param_1_type, param_1_value, param_2_type, param_2_value) \
  {									\
    type_st params[] = {						\
      {									\
         .type = strdup(#param_1_type),                                 \
         .value = &param_1_value,		       			\
         .size = sizeof(param_1_value),		       			\
      },								\
      {									\
         .type = strdup(#param_2_type),                                 \
         .value = &param_2_value,		       			\
         .size = sizeof(param_2_value),		       			\
      },								\
    };									\
    __mock_record(__func__, params, sizeof(params) / sizeof(type_st), MOCK_SKIP_T, NULL); \
  } while(0);

#define MOCK_RECORD_2_PARAM_RET(param_1_type, param_1_value, param_2_type, param_2_value, ret_type)) \
  {									\
    type_st ret;							\
    ret.type = strdup(#ret_type);                                       \
    type_st params[] = {						\
      {									\
         .type = strdup(#param_1_type),                                 \
         .value = &param_1_value,		       			\
         .size = sizeof(param_1_value),		       			\
      },								\
      {									\
         .type = strdup(#param_2_type),                                 \
         .value = &param_2_value,		       			\
         .size = sizeof(param_2_value),		       			\
      },								\
    };									\
    __mock_record(__func__, params, sizeof(params) / sizeof(type_st), #ret_type, &ret); \
    if (strcmp(ret.type, #ret_type) == 0) {				\
      return *((ret_type*)ret.value);					\
    }									\
    return (ret_type)0;							\
  } while(0);

void __mock_record(const char *function_name, type_st *params, size_t no_params, const char *recorded_ret_type, type_st *ret);

bool mock_is_succeeded(void);

#endif // TEST_MOCK_H_
