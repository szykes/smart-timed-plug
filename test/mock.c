#include "mock.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "framework.h"

mock_call_st mock_calls[NO_MOCK_CALLS];

void mock_clear_calls(void) {
  for (size_t i = 0; i < sizeof(mock_calls)/sizeof(mock_call_st); i++) {
    for ( size_t j = 0; j < sizeof(mock_calls[i].params)/sizeof(type_st); j++) {
      if (mock_calls[i].params[j].value != NULL) {
	free(mock_calls[i].params[j].value);
      }
    }
    if (mock_calls[i].ret.value != NULL) {
      free(mock_calls[i].ret.value);
    }
  }

  memset(&mock_calls, 0, sizeof(mock_calls));
}

void __mock_initiate_expectation(const char *function_name, type_st *params, size_t no_params, type_st *ret, const char *func, unsigned int line, const char *fmt, ...) {
  size_t i;
  for (i = 0; i < sizeof(mock_calls)/sizeof(mock_call_st); i++) {
    if (mock_calls[i].is_expected == false) {
      mock_calls[i].is_expected = true;

      if (sizeof(mock_calls[i].function_name) < (strlen(function_name) + 1)) {
	log_error("Function name too long, name: %s, len: %d", function_name, strlen(function_name));
	break;
      }
      strcpy(mock_calls[i].function_name, function_name);

      if (sizeof(mock_calls[i].params)/sizeof(type_st) < no_params) {
	log_error("Not enough space for parameters, function: %s, no_params: %d", function_name, no_params);
	break;
      }
      for (size_t j = 0; j < no_params; j++) {
	memcpy(&mock_calls[i].params[j], &params[j], sizeof(type_st));
      }

      if (ret != NULL) {
	mock_calls[i].ret = *ret;
      }

      int size = snprintf(mock_calls[i].place, sizeof(mock_calls[i].place), "%s:%d", func, line);
      if (size >= sizeof(mock_calls[i].place)) {
	log_error("Place buffer too small, actual: %d, but needed: %d", sizeof(mock_calls[i].place), size);
      }

      va_list arglist;
      va_start(arglist, fmt);
      size = vsnprintf(mock_calls[i].message, sizeof(mock_calls[i].message), fmt, arglist);
      if (size >= sizeof(mock_calls[i].message)) {
	log_error("Message buffer too small, actual: %d, but needed: %d", sizeof(mock_calls[i].message), size);
      }
      va_end(arglist);
      break;
    }
  }

  if (i >= sizeof(mock_calls)/sizeof(mock_call_st)) {
    log_error("No space for expecting mock call");
  }
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

static bool check_function_name(mock_call_st *mock_call, size_t idx, const char *function_name) {
  if (sizeof(mock_call->function_name) < (strlen(function_name) + 1)) {
    fill_result(mock_call, "Function name too long, name: %s, len: %d", function_name, strlen(function_name));
    return false;
  }

  if (strcmp(mock_call->function_name, function_name) != 0) {
    fill_result(mock_call, "Wrong function is expected here, expected: %s, got: %s", mock_call->function_name, function_name);
    return false;
  }
  return true;
}

static bool check_param(mock_call_st *mock_call, size_t idx, type_st *expected_param, size_t expected_param_idx, type_st *param) {
  if (expected_param->type == TYPE_NONE) {
    fill_result(mock_call, "No more parameters expected at function: %s, parameter index: %d, param_type: %d", mock_call->function_name, expected_param_idx, param->type);
    return false;
  }

  if (expected_param->type != param->type) {
    fill_result(mock_call, "Wrong parameter type at function: %s, parameter index: %d, expected: %d, got: %d", mock_call->function_name, expected_param_idx, expected_param->type, param->type);
    return false;
  }

  if (expected_param->size != param->size) {
    fill_result(mock_call, "Wrong parameter size at function: %s, parameter index: %d, expected: %d, got: %d", mock_call->function_name, expected_param_idx, expected_param->size, param->size);
    return false;
  }

  if (memcmp(expected_param->value, param->value, expected_param->size) != 0) {
    fill_result(mock_call, "Wrong parameter value at function: %s, parameter index: %d", mock_call->function_name, expected_param_idx);
    return false;
  }

  return true;
}

static bool check_all_params(mock_call_st *mock_call, size_t idx, type_st *params, size_t no_params) {
  if (sizeof(mock_call->params)/sizeof(type_st) < no_params) {
    fill_result(mock_call, "Not enough space for parameters, function: %s, no_params: %d", mock_call->function_name, no_params);
    return false;
  }

  for (size_t i = 0; i < no_params; i++) {
    if (!check_param(mock_call, idx, &mock_call->params[i], i, &params[i])) {
      return false;
    }
  }

  return true;
}

static void add_record(mock_call_st *mock_call, size_t idx, const char *function_name, type_st *params, size_t no_params, type_st *ret) {
  if (!check_function_name(mock_call, idx, function_name)) {
    return;
  }

  if (!check_all_params(mock_call, idx, params, no_params)) {
    return;
  }

  if (ret != NULL) {
    if (mock_call->ret.type == TYPE_NONE) {
      fill_result(mock_call, "No return value expected at function: %s, return type: %d", mock_call->function_name, ret->type);
      return;
    }

    memcpy(ret, &mock_call->ret, sizeof(mock_call->ret));
  }

  mock_call->is_matched = true;
}

void __mock_record(const char *function_name, type_st *params, size_t no_params, type_st *ret) {
  size_t i;
  for (i = 0; i < sizeof(mock_calls)/sizeof(mock_call_st); i++) {
    if (mock_calls[i].is_called == false) {
      mock_calls[i].is_called = true;

      add_record(&mock_calls[i], i, function_name, params, no_params, ret);

      break;
    }
  }

  if (i >= sizeof(mock_calls)/sizeof(mock_call_st)) {
    log_error("No space for recording mock call");
  }
}

bool mock_is_succeeded(void) {
  for (size_t i = 0; i < sizeof(mock_calls)/sizeof(mock_call_st); i++) {
    if (mock_calls[i].is_expected == false \
     && mock_calls[i].is_called == true) {
      log_fail("Mock call(s) not expected at [%d], %s, place: %s, message: %s", i, mock_calls[i].result, mock_calls[i].place, mock_calls[i].message);
      return false;
    }
    if (mock_calls[i].is_expected == true \
     && mock_calls[i].is_called == false) {
      log_fail("Mock call(s) missing call at [%d], function is not expected here, function: '%s()', place: %s, message: %s", i, mock_calls[i].function_name, mock_calls[i].place, mock_calls[i].message);
      return false;
    }
    if (mock_calls[i].is_expected == true \
     && mock_calls[i].is_called == true \
     && mock_calls[i].is_matched == false) {
      log_fail("Mock call(s) not matched at [%d], %s, place: %s, message: %s", i, mock_calls[i].result, mock_calls[i].place, mock_calls[i].message);
      return false;
    }
  }

  return true;
}
