/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/adapter/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/module_constants_h.h>

#include "thrift/compiler/test/fixtures/adapter/gen-cpp2/module_types.h"

namespace facebook::thrift::test {
/** Glean {"file": "thrift/compiler/test/fixtures/adapter/src/module.thrift"} */
namespace module_constants {

  /** Glean {"constant": "var1"} */
  ::apache::thrift::adapt_detail::adapted_t<MyVarAdapter, ::std::int32_t> const& var1();

  /** Glean {"constant": "var2"} */
  ::apache::thrift::adapt_detail::adapted_t<MyVarAdapter, ::std::string> const& var2();

  /** Glean {"constant": "var3"} */
  ::apache::thrift::adapt_detail::adapted_t<MyVarAdapter, ::facebook::thrift::test::MyStruct> const& var3();

  /** Glean {"constant": "var4"} */
  ::apache::thrift::adapt_detail::adapted_t<MyVarAdapter, ::std::int32_t> const& var4();

  /** Glean {"constant": "var5"} */
  ::apache::thrift::adapt_detail::adapted_t<MyVarAdapter, ::std::string> const& var5();

  /** Glean {"constant": "var6"} */
  ::apache::thrift::adapt_detail::adapted_t<MyVarAdapter, ::facebook::thrift::test::MyStruct> const& var6();

  /** Glean {"constant": "timeout"} */
  ::apache::thrift::adapt_detail::adapted_t<::apache::thrift::test::VariableAdapter, ::std::int32_t> const& timeout();

  /** Glean {"constant": "msg"} */
  ::apache::thrift::adapt_detail::adapted_t<::apache::thrift::test::VariableAdapter, ::std::string> const& msg();

  /** Glean {"constant": "person"} */
  ::apache::thrift::adapt_detail::adapted_t<::apache::thrift::test::VariableAdapter, ::facebook::thrift::test::Person2> const& person();

  /** Glean {"constant": "timeout_no_transitive"} */
  ::apache::thrift::adapt_detail::adapted_t<::apache::thrift::test::VariableAdapter, ::std::int32_t> const& timeout_no_transitive();

  /** Glean {"constant": "msg_no_transitive"} */
  ::apache::thrift::adapt_detail::adapted_t<::apache::thrift::test::VariableAdapter, ::std::string> const& msg_no_transitive();

  /** Glean {"constant": "person_no_transitive"} */
  ::apache::thrift::adapt_detail::adapted_t<::apache::thrift::test::VariableAdapter, ::facebook::thrift::test::Person2> const& person_no_transitive();

  /** Glean {"constant": "type_adapted"} */
  ::facebook::thrift::test::AdaptedBool const& type_adapted();

  /** Glean {"constant": "nested_adapted"} */
  ::facebook::thrift::test::MoveOnly const& nested_adapted();

  /** Glean {"constant": "container_of_adapted"} */
  ::std::vector<::facebook::thrift::test::AdaptedByte> const& container_of_adapted();

  FOLLY_EXPORT ::std::string_view _fbthrift_schema_6d60dbfef064445d();
  FOLLY_EXPORT ::folly::Range<const ::std::string_view*> _fbthrift_schema_6d60dbfef064445d_includes();
  FOLLY_EXPORT ::folly::Range<const ::std::string_view*> _fbthrift_schema_6d60dbfef064445d_uris();

} // namespace module_constants
} // namespace facebook::thrift::test
