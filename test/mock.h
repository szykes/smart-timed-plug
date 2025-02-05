#ifndef TEST_MOCK_H_
#define TEST_MOCK_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TYPE_NONE = 0,
  TYPE_UINT8_T,
  TYPE_UINT16_T,
  TYPE_SIZE_T,
  TYPE_BUTTON_EVENT_E,
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
  bool is_expected;
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

// expect
#define MOCK_EXPECT(function_name, fmt, ...)			\
  __mock_initiate_expectation(function_name, NULL, 0, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__);

#define MOCK_EXPECT_RET(function_name, ret_type, ret_value, fmt, ...)	\
  {									\
    void *ret_ptr;							\
    mock_prepare_param(ret_ptr, ret_value);				\
    type_st ret = {							\
      .type = ret_type,							\
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
         .type = param_1_type,	       					\
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
         .type = param_1_type,	       					\
         .value = param_1_ptr,						\
         .size = sizeof(param_1_value),		       			\
      }									\
    };									\
    type_st ret = {                                                     \
      .type = ret_type,                                                 \
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
         .type = param_1_type,	       					\
         .value = param_1_ptr,						\
         .size = sizeof(param_1_value),		       			\
      },								\
      {									\
         .type = param_2_type,	       					\
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
         .type = param_1_type,                                          \
         .value = param_1_ptr,                                          \
         .size = sizeof(param_1_value),                                 \
      },                                                                \
      {                                                                 \
         .type = param_2_type,                                          \
         .value = param_2_ptr,                                          \
         .size = sizeof(param_2_value),                                 \
      },                                                                \
    };                                                                  \
    __mock_initiate_expectation(function_name, params, sizeof(params) / sizeof(type_st), &ret, __func__, __LINE__, fmt, ##__VA_ARGS__); \
  } while(0);

void __mock_initiate_expectation(const char *function_name, type_st *params, size_t no_params, type_st *ret, const char *func, unsigned int line, const char *fmt, ...);

// record
#define MOCK_RECORD() \
  __mock_record(__func__, NULL, 0, NULL);

#define MOCK_RECORD_RET(ret_type, ret_cast)				\
  {									\
    type_st ret = {.type = TYPE_NONE};					\
    __mock_record(__func__, NULL, 0, &ret);				\
    if (ret.type == ret_type) {						\
      return *((ret_cast*)ret.value);					\
    }									\
    return 0;								\
  } while(0);

#define MOCK_RECORD_1_PARAM(param_1_type, param_1_value)		\
  {									\
    type_st params[] = {						\
      {									\
         .type = param_1_type,	       					\
         .value = &param_1_value,		       			\
         .size = sizeof(param_1_value),		       			\
      }									\
    };									\
    __mock_record(__func__, params, sizeof(params) / sizeof(type_st), NULL); \
  } while(0);

#define MOCK_RECORD_1_PARAM_RET(param_1_type, param_1_value, ret_type, ret_cast)	\
  {									\
    type_st ret = {.type = TYPE_NONE};					\
    type_st params[] = {						\
      {									\
         .type = param_1_type,	       					\
         .value = &param_1_value,		       			\
         .size = sizeof(param_1_value),		       			\
      }									\
    };									\
    __mock_record(__func__, params, sizeof(params) / sizeof(type_st), &ret); \
    if (ret.type == ret_type) {						\
      return *((ret_cast*)ret.value);					\
    }									\
    return 0;								\
  } while(0);

#define MOCK_RECORD_2_PARAM(param_1_type, param_1_value, param_2_type, param_2_value) \
  {									\
    type_st params[] = {						\
      {									\
         .type = param_1_type,	       					\
         .value = &param_1_value,		       			\
         .size = sizeof(param_1_value),		       			\
      },								\
      {									\
         .type = param_2_type,	       					\
         .value = &param_2_value,		       			\
         .size = sizeof(param_2_value),		       			\
      },								\
    };									\
    __mock_record(__func__, params, sizeof(params) / sizeof(type_st), NULL); \
  } while(0);

#define MOCK_RECORD_2_PARAM_RET(param_1_type, param_1_value, param_2_type, param_2_value, ret_type, ret_cast)) \
  {									\
    type_st ret = {.type = TYPE_NONE};                                  \
    type_st params[] = {						\
      {									\
         .type = param_1_type,	       					\
         .value = &param_1_value,		       			\
         .size = sizeof(param_1_value),		       			\
      },								\
      {									\
         .type = param_2_type,	       					\
         .value = &param_2_value,		       			\
         .size = sizeof(param_2_value),		       			\
      },								\
    };									\
    __mock_record(__func__, params, sizeof(params) / sizeof(type_st), &ret); \
    if (ret.type == ret_type) {						\
      return *((ret_cast*)ret.value);					\
    }									\
    return 0;								\
  } while(0);

void __mock_record(const char *function_name, type_st *params, size_t no_params, type_st *ret);

bool mock_is_succeeded(void);

#endif // TEST_MOCK_H_
