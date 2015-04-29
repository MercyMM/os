struct A;
struct B;

struct A a1;
struct A a2;
struct A *a3;
struct A *a4;
struct A **a5;
struct A **a6;
struct A *a6[1];
struct A *a7[1];
struct A *a7[1][2];
struct A **a8[1][2];
struct A **a9[1][2];
struct A (*a10)[1];
struct A (*a11)[1];
struct A (*a12)[1][2];
struct A (**a14)[1][2];
struct A (**a15)[1][2];

union A;
union B;

enum A a1;
enum A a2;
enum A *a3;
enum A *a4;
enum A **a5;
enum A **a6;
enum A *a6[1];
enum A *a7[1];
enum A *a7[1][2];
enum A **a8[1][2];
enum A **a9[1][2];
enum A (*a10)[1];
enum A (*a11)[1];
enum A (*a12)[1][2];
enum A (**a14)[1][2];
enum A (**a15)[1][2];
