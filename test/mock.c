#include "mock.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "framework.h"

typedef struct mock_uninterested_call_st {
  char *func;
  struct mock_uninterested_call_st *next;
} mock_uninterested_call_st;

static mock_uninterested_call_st *mock_uninterested_call_head;
static mock_uninterested_call_st mock_uninterested_call_dummy_head = {
  .next = NULL,
};

static mock_call_st *mock_calls_head;
static mock_call_st mock_calls_dummy_head = {
  .next = NULL,
};

void mock_init(void) {
  mock_calls_head = &mock_calls_dummy_head;
  mock_uninterested_call_head = &mock_uninterested_call_dummy_head;
}

void mock_uninterested_call(const char *function_name) {
  mock_uninterested_call_st *curr;
  curr = mock_uninterested_call_head;
  for (; curr->next != NULL; ) {
    curr = curr->next;
  }

  curr->next = malloc(sizeof(mock_uninterested_call_st));
  memset(curr->next, 0, sizeof(mock_uninterested_call_st));
  curr = curr->next;
  curr->next = NULL;

  curr->func = strdup(function_name);
}

void mock_clear_all_uninterested_calls(void) {
  for (mock_uninterested_call_st *curr = mock_uninterested_call_head->next; curr != NULL;) {
    mock_uninterested_call_st *next = curr->next;
    free(curr->func);
    free(curr);
    curr = next;
  }

  mock_uninterested_call_head->next = NULL;
}

void mock_clear_calls(void) {
  for (mock_call_st *curr = mock_calls_head->next; curr != NULL;) {
    mock_call_st *next = curr->next;

    for (size_t i = 0; i < sizeof(curr->params)/sizeof(type_st); i++) {
      if (curr->params[i].value != NULL) {
	free(curr->params[i].value);
	free(curr->params[i].type);
      }
    }

    if (curr->ret.value != NULL) {
      free(curr->ret.value);
      free(curr->ret.type);
    }

    free(curr);

    curr = next;
  }

  mock_calls_head->next = NULL;
}

static mock_call_st * add_new_mock(const char *function_name, type_st *params, size_t no_params, type_st *ret) {
  mock_call_st *curr;
  curr = mock_calls_head;
  for (; curr->next != NULL; ) {
    curr = curr->next;
  }

  curr->next = malloc(sizeof(mock_call_st));
  memset(curr->next, 0, sizeof(mock_call_st));
  curr = curr->next;
  curr->next = NULL;

  if (sizeof(curr->function_name) < (strlen(function_name) + 1)) {
    log_error("Function name too long, name: %s(), len: %d", function_name, strlen(function_name));
    return NULL;
  }
  strcpy(curr->function_name, function_name);

  if (sizeof(curr->params)/sizeof(type_st) < no_params) {
    log_error("Not enough space for parameters, function: %s(), no_params: %d", function_name, no_params);
    return NULL;
  }
  for (size_t j = 0; j < no_params; j++) {
    memcpy(&curr->params[j], &params[j], sizeof(type_st));
  }

  if (ret != NULL) {
    curr->ret = *ret;
    if (strcmp(curr->ret.type, MOCK_SKIP_T) == 0) {
      log_error("Return type cannot be skipped, it must be a valid type, function: %s()", function_name);
    }
  }

  return curr;
}

static bool is_uninterested_call(const char* function_name) {
  mock_uninterested_call_st *curr;
  for (curr = mock_uninterested_call_head->next; curr != NULL; curr = curr->next) {
    if (strcmp(curr->func, function_name) == 0) {
      return true;
    }
  }
  return false;
}

void __mock_initiate_expectation(const char *function_name, type_st *params, size_t no_params, type_st *ret, const char *func, unsigned int line, const char *fmt, ...) {
  if (is_uninterested_call(function_name)) {
    log_fail("Marked as uninterested call but an execptation initiated, function: %s()", function_name);
    exit(1);
  }

  mock_call_st *curr = add_new_mock(function_name, params, no_params, ret);
  if (curr == NULL) {
    log_fail("NULL ptr");
    exit(1);
  }

  curr->is_expected = true;

  int size = snprintf(curr->place, sizeof(curr->place), "%s:%d", func, line);
  if (size >= sizeof(curr->place)) {
    log_error("Place buffer too small, actual: %d, but needed: %d", sizeof(curr->place), size);
  }

  va_list arglist;
  va_start(arglist, fmt);
  size = vsnprintf(curr->message, sizeof(curr->message), fmt, arglist);
  if (size >= sizeof(curr->message)) {
    log_error("Message buffer too small, actual: %d, but needed: %d", sizeof(curr->message), size);
  }
  va_end(arglist);
}

static void fill_result(mock_call_st *mock_call, const char *fmt, ...) {
  va_list arglist;
  va_start(arglist, fmt);
  int result_size = vsnprintf(mock_call->result, sizeof(mock_call->result), fmt, arglist);
  va_end(arglist);

  if (result_size >= sizeof(mock_call->result)) {
    log_error("Result buffer too small, actual: %d, but needed: %d", sizeof(mock_call->result), result_size);
  }
}

static bool check_function_name(mock_call_st *mock_call, const char *function_name) {
  if (sizeof(mock_call->function_name) < (strlen(function_name) + 1)) {
    fill_result(mock_call, "Function name too long, name: %s(), len: %d", function_name, strlen(function_name));
    return false;
  }

  if (strcmp(mock_call->function_name, function_name) != 0) {
    fill_result(mock_call, "Wrong function is expected here, expected: %s(), got: %s()", mock_call->function_name, function_name);
    return false;
  }
  return true;
}

static bool check_param(mock_call_st *mock_call, type_st *expected_param, size_t expected_param_idx, type_st *param) {
  if (expected_param->type == NULL) {
    fill_result(mock_call, "No more parameters expected at function: %s(), parameter index: %d, param_type: %d", mock_call->function_name, expected_param_idx, param->type);
    return false;
  }

  if (strcmp(expected_param->type, MOCK_SKIP_T) == 0) {
    return true;
  }

  if (strcmp(param->type, MOCK_SKIP_T) == 0) {
    fill_result(mock_call, "If the param type at record is %s, the expected cannot be different at function: %s(), parameter: %d, expected: %s", MOCK_SKIP_T, mock_call->function_name, expected_param_idx, expected_param->type, param->type);
    return false;
  }

  if (strcmp(expected_param->type, param->type) != 0) {
    fill_result(mock_call, "Wrong parameter type at function: %s(), parameter: %d, expected: %s, got: %s", mock_call->function_name, expected_param_idx, expected_param->type, param->type);
    return false;
  }

  if (expected_param->size != param->size) {
    fill_result(mock_call, "Wrong parameter size at function: %s(), parameter index: %d, expected: %d, got: %d", mock_call->function_name, expected_param_idx, expected_param->size, param->size);
    return false;
  }

  if (memcmp(expected_param->value, param->value, expected_param->size) != 0) {
    fill_result(mock_call, "Wrong parameter value at function: %s(), parameter index: %d", mock_call->function_name, expected_param_idx);
    return false;
  }

  return true;
}

static bool check_all_params(mock_call_st *mock_call, type_st *params, size_t no_params) {
  if (sizeof(mock_call->params)/sizeof(type_st) < no_params) {
    fill_result(mock_call, "Not enough space for parameters, function: %s(), no_params: %d", mock_call->function_name, no_params);
    return false;
  }

  for (size_t i = 0; i < no_params; i++) {
    if (!check_param(mock_call, &mock_call->params[i], i, &params[i])) {
      return false;
    }
  }

  return true;
}

static void add_record(mock_call_st *mock_call, const char *function_name, type_st *params, size_t no_params, const char* recorded_ret_type, type_st *ret) {
  if (!check_function_name(mock_call, function_name)) {
    return;
  }

  if (!check_all_params(mock_call, params, no_params)) {
    return;
  }

  if (ret != NULL) {
    if (mock_call->ret.type == NULL) {
      fill_result(mock_call, "No return value expected at function: %s(), return type: %d", mock_call->function_name, ret->type);
      return;
    }

    memcpy(ret, &mock_call->ret, sizeof(mock_call->ret));

    if (strcmp(recorded_ret_type, MOCK_SKIP_T) == 0) {
      fill_result(mock_call, "Return type cannot be skipped, it must be valid type at function: %s()", mock_call->function_name);
      return;
    }

    if (strcmp(recorded_ret_type, ret->type) != 0) {
      fill_result(mock_call, "Mismatching return types at function: %s(), recorded return type: %s, expected return type: %s", mock_call->function_name, recorded_ret_type, ret->type);
      return;
    }
  }

  mock_call->is_matched = true;
}

static void print_mocks(void) {
  size_t i = 0;
  for (mock_call_st *curr = mock_calls_head->next; curr != NULL; curr = curr->next) {
    if (curr->is_called == false) {
      log_fail("Mock call[%d]: expected but not called, func: %s(), place: '%s', message: '%s'", i, curr->function_name, curr->place, curr->message);
    } else if (!curr->is_expected) {
      log_fail("Mock call[%d]: not expected but called, func: %s()", i, curr->function_name);
    } else if (curr->is_called == true && curr->is_matched == false) {
      log_fail("Mock call[%d]: not matched, place: '%s', result: '%s', message: '%s'", i, curr->result, curr->place, curr->message);
    } else {
      log_info("Mock call[%d]: ok, func: %s(), message: '%s'", i, curr->function_name, curr->message);
    }
    i++;
  }
}

void __mock_record(const char *function_name, type_st *params, size_t no_params, const char *recorded_ret_type, type_st *ret) {
  mock_call_st *curr;
  for (curr = mock_calls_head->next; curr != NULL; curr = curr->next) {
    if (curr->is_called == false) {
      if (is_uninterested_call(function_name)) {
	return;
      }

      curr->is_called = true;

      add_record(curr, function_name, params, no_params, recorded_ret_type, ret);

      return;
    }
  }

  curr = add_new_mock(function_name, params, no_params, ret);
  if (curr == NULL) {
    log_fail("NULL ptr");
    exit(1);
  }

  curr->is_matched = false;
  curr->is_called = true;
}

bool mock_is_succeeded(void) {
  bool is_passed = true;

  for (mock_call_st *curr = mock_calls_head->next; curr != NULL; curr = curr->next) {
    if (!curr->is_called ||
	(curr->is_called && !curr->is_matched) ||
	!curr->is_expected) {
      is_passed = false;
      break;
    }
  }

  if (!is_passed) {
    print_mocks();
  }

  return is_passed;
}
