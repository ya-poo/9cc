#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
    return a+b+c+d+e+f;
}
EOF

assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" >tmp.s
    cc -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 'main() { return 0; }'
assert 42 'main() { return 42; }'
assert 21 'main() { return 5+20-4; }'
assert 41 'main() { return  12 + 34 - 5 ; }'
assert 47 'main() { return 5+6*7; }'
assert 15 'main() { return 5*(9-6); }'
assert 4 'main() { return (3+5)/2; }'
assert 10 'main() { return -10+20; }'
assert 10 'main() { return - -10; }'
assert 10 'main() { return - - +10; }'

assert 0 'main() { return 0==1; }'
assert 1 'main() { return 42==42; }'
assert 1 'main() { return 0!=1; }'
assert 0 'main() { return 42!=42; }'

assert 1 'main() { return 0<1; }'
assert 0 'main() { return 1<1; }'
assert 0 'main() { return 2<1; }'
assert 1 'main() { return 0<=1; }'
assert 1 'main() { return 1<=1; }'
assert 0 'main() { return 2<=1; }'

assert 1 'main() { return 1>0; }'
assert 0 'main() { return 1>1; }'
assert 0 'main() { return 1>2; }'
assert 1 'main() { return 1>=0; }'
assert 1 'main() { return 1>=1; }'
assert 0 'main() { return 1>=2; }'

assert 3 'main() { 1; 2; return 3; }'
assert 3 'main() { a=3; return a; }'
assert 3 'main() { a=27; b=3; c=10; return (a + b)/c; }'
assert 5 'main() { a=3; return 5; }'
assert 26 'main() { a=1; b=a+1; c=b+1; d=c+1; e=d+1; f=e+1; g=f+1; h=g+1; i=h+1; j=i+1; k=j+1; l=k+1; m=l+1; n=m+1; o=n+1; p=o+1; q=p+1; r=q+1; s=r+1; t=s+1; u=t+1; v=u+1; w=v+1; x=w+1; y=x+1; return z=y+1; }'

assert 8 'main() { foo123ABC=3; bar=5; return foo123ABC+bar; }'
assert 3 'main() { _foo123=3; return _foo123; }'

assert 1 'main() { return 1; 2; 3; }'
assert 2 'main() { 1; return 2; 3; }'
assert 1 'main() { return 1; return 2; }'
assert 4 'main() { a=1; b=3; return a+b; }'

assert 3 'main () { if (0) return 2; return 3; }'
assert 3 'main() { if (1-1) return 2; return 3; }'
assert 2 'main() { if (1) return 2; return 3; }'
assert 2 'main() { if (2-1) return 2; return 3; }'

assert 2 'main() { if (1) return 2; else return 3; }'
assert 3 'main() { if (0) return 2; else return 3; }'

assert 10 'main() { i=0; while(i<10) i=i+1; return i; }'

assert 55 "main() {
    sum = 0;
    for(i=1; i<=10; i=i+1)
        sum = sum + i;
    return sum;
}"

assert 3 'main () { 1; {2;} return 3; }'
assert 55 'main() { i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'
assert 233 "main() {
    i = 1;
    j = 1;
    z = 0;
    for(n=3; n<=13; n=n+1) {
        z = i + j;
        i = j;
        j = z;
    }
    return z;
}"

assert 3 'main() { return ret3(); }'
assert 9 'main() { a = 4; return a + ret5(); }'
assert 8 'main() { return add(3, 5); }'
assert 2 'main() { return sub(5, 3); }'
assert 21 'main() { return add6(1,2,3,4,5,6); }'

assert 32 'main() { return ret32(); } ret32() { return 32; }'

assert 7 'main() { return add2(3,4); } add2(x,y) { return x+y; }'
assert 1 'main() { return sub2(4,3); } sub2(x,y) { return x-y; }'
assert 55 'main() { return fib(9); } fib(x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

assert 3 'main() { x=3; return *&x; }'
assert 3 'main() { x=3; y=&x; z=&y; return **z; }'
assert 5 'main() { x=3; y=5; return *(&x-8); }'
assert 3 'main() { x=3; y=5; return *(&y+8); }'
assert 5 'main() { x=3; y=&x; *y=5; return x; }'
assert 7 'main() { x=3; y=5; *(&x-8)=7; return y; }'
assert 7 'main() { x=3; y=5; *(&y+8)=7; return x; }'

echo OK
