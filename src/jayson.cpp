#include "jayson.hpp"
#include <cstdlib>
#include <variant>

#include "cJSON.h"

Json::Json(cJSON* root, bool owned) {
  this->root = root;
  this->owned = owned;
  this->ref_count = static_cast<int*>(malloc(sizeof(int)));
  *this->ref_count = 1;
  this->str_invalid = true;
}

Json& Json::operator=(const Json& other) {
  if (this == &other) {
    return *this;
  }
  this->release();
  this->root = other.root;
  this->ref_count = other.ref_count;
  this->owned = other.owned;
  *this->ref_count = *this->ref_count + 1;
  return *this;
}

Json::Json(const Json& other) {
  this->root = other.root;
  this->ref_count = other.ref_count;
  this->owned = other.owned;
  *ref_count = *ref_count + 1;
}

Json::~Json() {
  this->release();
}

JsonObject::JsonObject(cJSON* root, bool owned) : Json(root, owned) {}

void Json::release() {
  int ref_count = *this->ref_count;
  if (ref_count > 1) {
    *this->ref_count = ref_count - 1;
  } else if (owned) {
    free(this->ref_count);
    cJSON_Delete(this->root);
  } else {
    free(this->ref_count);
  }
}

std::variant<JsonObject, JsonError> JsonObject::parse(char* str) {
  auto root = cJSON_Parse(str);
  if (root == nullptr || !cJSON_IsObject(root)) {
    return JsonError::PARSE_ERROR;
  } else {
    JsonObject json(root, true);
    return json;
  }
}

JsonObject::JsonObject() : JsonObject(cJSON_CreateObject(), true) {}

void JsonObject::set(const char* name, const char* string) {
  str_invalid = true;
  auto item = cJSON_GetObjectItem(this->root, name);
  if (item && cJSON_IsString(item)) {
    cJSON_SetValuestring(item, string);
  } else if (item) {
    cJSON_ReplaceItemInObject(this->root, name, cJSON_CreateString(string));
  } else {
    cJSON_AddStringToObject(this->root, name, string);
  }
}

void JsonObject::set(const char* name, JsonObject* object) {
  str_invalid = true;
  cJSON* copy = cJSON_Duplicate(object->root, true);
  if (!copy)
    return;
  auto item = cJSON_GetObjectItem(this->root, name);
  if (item) {
    cJSON_ReplaceItemInObject(this->root, name, copy);
  } else {
    cJSON_AddItemToObject(this->root, name, copy);
  }
}

void JsonObject::add_object(const char* name) {
  str_invalid = true;
  auto object = cJSON_GetObjectItem(this->root, name);
  if (object && cJSON_IsObject(object)) {
    return;
  }
  cJSON_AddObjectToObject(this->root, name);
}

void JsonObject::set(const char* name, JsonArray* array) {
  str_invalid = true;
  cJSON* copy = cJSON_Duplicate(array->root, true);
  if (!copy) {
    return;
  }
  auto item = cJSON_GetObjectItem(this->root, name);
  if (item) {
    cJSON_ReplaceItemInObject(this->root, name, copy);
  } else {
    cJSON_AddItemToObject(this->root, name, copy);
  }
}

void JsonObject::set(const char* name, bool boolean) {
  str_invalid = true;
  auto item = cJSON_GetObjectItem(this->root, name);
  if (item && cJSON_IsBool(item)) {
    cJSON_SetBoolValue(item, boolean);
  } else if (item) {
    cJSON_ReplaceItemInObject(this->root, name, cJSON_CreateBool(boolean));
  } else {
    cJSON_AddBoolToObject(this->root, name, boolean);
  }
}

void JsonObject::set(const char* name, JsonError error) {
  str_invalid = true;
  const char* error_message = "";
  switch (error) {
    case JsonError::NOT_PROVIDED:
      error_message = "not provided";
      break;
    case JsonError::NOT_A_NUMBER:
      error_message = "not a number";
      break;
    case JsonError::NOT_A_STRING:
      error_message = "not a string";
      break;
    case JsonError::NOT_AN_OBJECT:
      error_message = "not an object";
      break;
    case JsonError::NOT_AN_ARRAY:
      error_message = "not an array";
      break;
    case JsonError::PARSE_ERROR:
      error_message = "parse error";
      break;
  }
  this->set(name, error_message);
}

std::variant<JsonObject, JsonError> JsonObject::get_object(const char* name) {
  auto item = cJSON_GetObjectItem(this->root, name);
  if (item == nullptr) {
    return JsonError::NOT_PROVIDED;
  } else if (!cJSON_IsObject(item)) {
    return JsonError::NOT_AN_OBJECT;
  } else {
    return JsonObject(item, false);
  }
}

std::variant<JsonObject, JsonError> JsonObject::get_object(const char* name,
                                                           JsonObject* error_object) {
  auto child_object = this->get_object(name);
  if (std::holds_alternative<JsonError>(child_object)) {
    error_object->set(name, std::get<JsonError>(child_object));
  };
  return child_object;
};

std::variant<double, JsonError> JsonObject::get_number(const char* name) {
  auto item = cJSON_GetObjectItem(this->root, name);
  if (item == nullptr) {
    return JsonError::NOT_PROVIDED;
  } else if (!cJSON_IsNumber(item)) {
    return JsonError::NOT_A_NUMBER;
  } else {
    return cJSON_GetNumberValue(item);
  }
}

std::variant<double, JsonError> JsonObject::get_number(const char* name, JsonObject* error_object) {
  auto number = this->get_number(name);
  if (std::holds_alternative<JsonError>(number)) {
    error_object->set(name, std::get<JsonError>(number));
  }
  return number;
}

std::variant<char*, JsonError> JsonObject::get_string(const char* name) {
  auto item = cJSON_GetObjectItem(this->root, name);
  if (item == nullptr) {
    return JsonError::NOT_PROVIDED;
  } else if (!cJSON_IsString(item)) {
    return JsonError::NOT_A_STRING;
  } else {
    return cJSON_GetStringValue(item);
  }
}

std::variant<char*, JsonError> JsonObject::get_string(const char* name, JsonObject* error_object) {
  auto string = this->get_string(name);
  if (std::holds_alternative<JsonError>(string)) {
    error_object->set(name, std::get<JsonError>(string));
  }
  return string;
}

bool JsonObject::is_empty() {
  return cJSON_IsObject(this->root) && this->root->child == nullptr;
}

std::string Json::stringify() {
  if (!str_invalid) {
    return str;
  }
  auto c_str = cJSON_PrintUnformatted(this->root);
  str = std::string(c_str);
  free(c_str);
  str_invalid = false;
  return str;
}

JsonArray::JsonArray(cJSON* root, bool owned) : Json(root, owned) {}

JsonArray::JsonArray() : Json(cJSON_CreateArray(), true) {}

std::variant<JsonArray, JsonError> JsonArray::parse(char* str) {
  auto root = cJSON_Parse(str);
  if (root == nullptr || !cJSON_IsArray(root)) {
    return JsonError::PARSE_ERROR;
  } else {
    JsonArray array(root, true);
    return array;
  }
}

int JsonArray::len() {
  return cJSON_GetArraySize(this->root);
}

void JsonArray::append_object(JsonObject* object) {
  str_invalid = true;
  auto copy = cJSON_Duplicate(object->root, true);
  if (!copy) {
    return;
  }
  cJSON_AddItemToArray(this->root, copy);
};