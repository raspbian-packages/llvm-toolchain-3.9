// RUN: %check_clang_tidy %s modernize-use-nullptr %t -- \
// RUN:   -config="{CheckOptions: [{key: modernize-use-nullptr.NullMacros, value: 'MY_NULL,NULL'}]}" \
// RUN:   -- -std=c++11

#define NULL 0

namespace std {

typedef decltype(nullptr) nullptr_t;

} // namespace std

// Just to make sure make_null() could have side effects.
void external();

std::nullptr_t make_null() {
  external();
  return nullptr;
}

void func() {
  void *CallTest = make_null();

  int var = 1;
  void *CommaTest = (var+=2, make_null());

  int *CastTest = static_cast<int*>(make_null());
}

void dummy(int*) {}
void side_effect() {}

#define MACRO_EXPANSION_HAS_NULL \
  void foo() { \
    dummy(0); \
    dummy(NULL); \
    side_effect(); \
  }

MACRO_EXPANSION_HAS_NULL;
#undef MACRO_EXPANSION_HAS_NULL


void test_macro_expansion1() {
#define MACRO_EXPANSION_HAS_NULL \
  dummy(NULL); \
  side_effect();

  MACRO_EXPANSION_HAS_NULL;

#undef MACRO_EXPANSION_HAS_NULL
}

// Test macro expansion with cast sequence, PR15572.
void test_macro_expansion2() {
#define MACRO_EXPANSION_HAS_NULL \
  dummy((int*)0); \
  side_effect();

  MACRO_EXPANSION_HAS_NULL;

#undef MACRO_EXPANSION_HAS_NULL
}

void test_macro_expansion3() {
#define MACRO_EXPANSION_HAS_NULL \
  dummy(NULL); \
  side_effect();

#define OUTER_MACRO \
  MACRO_EXPANSION_HAS_NULL; \
  side_effect();

  OUTER_MACRO;

#undef OUTER_MACRO
#undef MACRO_EXPANSION_HAS_NULL
}

void test_macro_expansion4() {
#define MY_NULL NULL
  int *p = MY_NULL;
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: use nullptr [modernize-use-nullptr]
  // CHECK-FIXES: int *p = nullptr;
#undef MY_NULL
}

#define IS_EQ(x, y) if (x != y) return;
void test_macro_args() {
  int i = 0;
  int *Ptr;

  IS_EQ(static_cast<int*>(0), Ptr);
  // CHECK-MESSAGES: :[[@LINE-1]]:27: warning: use nullptr
  // CHECK-FIXES: IS_EQ(static_cast<int*>(nullptr), Ptr);

  IS_EQ(0, Ptr);    // literal
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: use nullptr
  // CHECK-FIXES: IS_EQ(nullptr, Ptr);

  IS_EQ(NULL, Ptr); // macro
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: use nullptr
  // CHECK-FIXES: IS_EQ(nullptr, Ptr);

  // These are ok since the null literal is not spelled within a macro.
#define myassert(x) if (!(x)) return;
  myassert(0 == Ptr);
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: use nullptr
  // CHECK-FIXES: myassert(nullptr == Ptr);

  myassert(NULL == Ptr);
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: use nullptr
  // CHECK-FIXES: myassert(nullptr == Ptr);

  // These are bad as the null literal is buried in a macro.
#define BLAH(X) myassert(0 == (X));
#define BLAH2(X) myassert(NULL == (X));
  BLAH(Ptr);
  BLAH2(Ptr);

  // Same as above but testing extra macro expansion.
#define EXPECT_NULL(X) IS_EQ(0, X);
#define EXPECT_NULL2(X) IS_EQ(NULL, X);
  EXPECT_NULL(Ptr);
  EXPECT_NULL2(Ptr);

  // Almost the same as above but now null literal is not in a macro so ok
  // to transform.
#define EQUALS_PTR(X) IS_EQ(X, Ptr);
  EQUALS_PTR(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: use nullptr
  // CHECK-FIXES: EQUALS_PTR(nullptr);
  EQUALS_PTR(NULL);
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: use nullptr
  // CHECK-FIXES: EQUALS_PTR(nullptr);

  // Same as above but testing extra macro expansion.
#define EQUALS_PTR_I(X) EQUALS_PTR(X)
  EQUALS_PTR_I(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: use nullptr
  // CHECK-FIXES: EQUALS_PTR_I(nullptr);
  EQUALS_PTR_I(NULL);
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: use nullptr
  // CHECK-FIXES: EQUALS_PTR_I(nullptr);

  // Ok since null literal not within macro. However, now testing macro
  // used as arg to another macro.
#define decorate(EXPR) side_effect(); EXPR;
  decorate(IS_EQ(NULL, Ptr));
  // CHECK-MESSAGES: :[[@LINE-1]]:18: warning: use nullptr
  // CHECK-FIXES: decorate(IS_EQ(nullptr, Ptr));
  decorate(IS_EQ(0, Ptr));
  // CHECK-MESSAGES: :[[@LINE-1]]:18: warning: use nullptr
  // CHECK-FIXES: decorate(IS_EQ(nullptr, Ptr));

  // This macro causes a NullToPointer cast to happen where 0 is assigned to z
  // but the 0 literal cannot be replaced because it is also used as an
  // integer in the comparison.
#define INT_AND_PTR_USE(X) do { int *z = X; if (X == 4) break; } while(false)
  INT_AND_PTR_USE(0);

  // Both uses of X in this case result in NullToPointer casts so replacement
  // is possible.
#define PTR_AND_PTR_USE(X) do { int *z = X; if (X != z) break; } while(false)
  PTR_AND_PTR_USE(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:19: warning: use nullptr
  // CHECK-FIXES: PTR_AND_PTR_USE(nullptr);
  PTR_AND_PTR_USE(NULL);
  // CHECK-MESSAGES: :[[@LINE-1]]:19: warning: use nullptr
  // CHECK-FIXES: PTR_AND_PTR_USE(nullptr);

#define OPTIONAL_CODE(...) __VA_ARGS__
#define NOT_NULL dummy(0)
#define CALL(X) X
  OPTIONAL_CODE(NOT_NULL);
  CALL(NOT_NULL);

#define ENTRY(X) {X}
  struct A {
    int *Ptr;
  } a[2] = {ENTRY(0), {0}};
  // CHECK-MESSAGES: :[[@LINE-1]]:19: warning: use nullptr
  // CHECK-MESSAGES: :[[@LINE-2]]:24: warning: use nullptr
  // CHECK-FIXES: a[2] = {ENTRY(nullptr), {nullptr}};
#undef ENTRY

#define assert1(expr) (expr) ? 0 : 1
#define assert2 assert1
  int *p;
  assert2(p == 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: use nullptr
  // CHECK-FIXES: assert2(p == nullptr);
  assert2(p == NULL);
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: use nullptr
  // CHECK-FIXES: assert2(p == nullptr);
#undef assert2
#undef assert1

#define ASSERT_EQ(a, b) a == b
#define ASSERT_NULL(x) ASSERT_EQ(static_cast<void *>(NULL), x)
  int *pp;
  ASSERT_NULL(pp);
  ASSERT_NULL(NULL);
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: use nullptr
  // CHECK-FIXES: ASSERT_NULL(nullptr);
#undef ASSERT_NULL
#undef ASSERT_EQ
}

// One of the ancestor of the cast is a NestedNameSpecifierLoc.
class NoDef;
char function(NoDef *p);
#define F(x) (sizeof(function(x)) == 1)
template<class T, T t>
class C {};
C<bool, F(0)> c;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: use nullptr
// CHECK-FIXES: C<bool, F(nullptr)> c;
#undef F
