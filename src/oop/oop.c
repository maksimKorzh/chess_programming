/*
    Mimic OOP approach in plain C
*/

// libraries
#include <stdio.h>

// structure that mimics "class" behavior
typedef struct
{
    // "class" inner fields
    int a;
    int b;
    
    // "class" methods
    int (*add)(int a, int b);
    int (*sub)(int a, int b);
}
MyClass;

// add "class" method
int add(int a, int b)
{
    return a + b;
}

// sub "class" method
int sub(int a, int b)
{
    return a - b;
}

// init "constructor"
void init(MyClass *self, int a, int b)
{
    // init inner "class" structure variables
    self->a = a;
    self->b = b;
    
    // init inner "class" structure methods
    self->add = add;
    self->sub = sub; 
}

// main driver
int main()
{
    // create "class" structure instance
    MyClass my_class;
    
    // init "constructor" of our "class" structure
    init(&my_class, 20, 10);
    
    printf("a: %d\n", my_class.a);
    printf("b: %d\n", my_class.b);
    
    int sum = my_class.add(my_class.a, my_class.b);
    printf("sum: %d\n", sum);
    
    int sub = my_class.sub(my_class.a, my_class.b);
    printf("sub: %d\n", sub);

    return 0;
}


















