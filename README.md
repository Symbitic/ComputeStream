# ComputeStream Demo

This is a prototype for a service to allow a server to receive and execute computing kernels, thus reducing the need for everyday users to buy their own graphics cards.

## Getting Started

The following must be installed first:

* CMake
* OpenCL
* Boost
* gRPC

## Building

Build the project by running:

```bash
git clone https://github.com/Symbitic/computestream
mkdir computestream/build
cd computestream/build
cmake ..
make
```

---

```
curl --location --request POST 'localhost:8848/create' \
--header 'Content-Type: application/json' \
--data-raw '{
  "source": "__kernel void add(__global const uint *a, __global const uint *b, __global uint *c) { const uint i = get_global_id(0); c[i] = a[i] + b[i]; }",
  "type": 0,
  "outputs": [ 16 ]
}'

curl --location --request GET 'localhost:8848/b4a5c0fa3a842bc79e2b3ee717dd260f8d854d0bab00dcd1d6f20593c47ee8e7'

curl --location --request PUT 'localhost:8848/update/b4a5c0fa3a842bc79e2b3ee717dd260f8d854d0bab00dcd1d6f20593c47ee8e7' \
--header 'Content-Type: application/json' \
--data-raw '{
  "update": "input",
  "index": 0,
  "data": [1,2,3,4]
}'

curl --location --request PUT 'localhost:8848/update/b4a5c0fa3a842bc79e2b3ee717dd260f8d854d0bab00dcd1d6f20593c47ee8e7' \
--header 'Content-Type: application/json' \
--data-raw '{
  "update": "input",
  "index": 0,
  "data": [5,6,7,8]
}'

curl --location --request GET 'localhost:8848/compute/b4a5c0fa3a842bc79e2b3ee717dd260f8d854d0bab00dcd1d6f20593c47ee8e7'
```
