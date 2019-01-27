# Radia (trimmed)

3D Magnetostatics Computer Code: https://github.com/ochubar/Radia

## Notes

To build clean (with 4 cores):

```sh
make clean; make -j4 core && make pylib
```

To make it easy to develop, symlink `radia.so` in install directory:

```sh
x=$(python -c 'import sys; from distutils.sysconfig import get_python_lib as g; sys.stdout.write(g())')/radia.so
rm -f $x
ln -s $PWD/cpp/gcc/radia.so $x
```

To compile a single file (`radintrc.cpp`):

```sh
$ rm cpp/gcc/{radintrc.o,radia.so,libradia.a}; make core && make pylib
```

Dependencies don't work so you have to delete the `.o`.

To make it easy to develop, do this:

```sh
x=$(python -c 'import sys; from distutils.sysconfig import get_python_lib as g; sys.stdout.write(g())')/radia.so
rm -f $x
ln -s $PWD/cpp/gcc/radia.so $x
```

To run the test with MPI with one worker (and one collector):

```
mpiexec -n 2 python MPMD_quad_YC1114.py
```

This should be approximately equivalent to serial run of ochubar/Radia.
