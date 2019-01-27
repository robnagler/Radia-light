# Radia (trimmed)

3D Magnetostatics Computer Code: https://github.com/ochubar/Radia

## Notes

To compile a single file (`radintrc.cpp`):

```sh
$ rm cpp/gcc/{radintrc.o,radia.so,libradia.a}; make -j4 core && make pylib
```

Dependencies don't work so you have to delete the `.o`.

To run the test with MPI with one worker (and one collector):

```
mpiexec -n 2 python MPMD_quad_YC1114.py
```

This should be approximately equivalent to serial run of ochubar/Radia.
