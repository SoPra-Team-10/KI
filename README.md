[<img src="https://travis-ci.org/SoPra-Team-10/KI.svg?branch=master" alt="Build Status">](https://travis-ci.org/SoPra-Team-10/KI)
# KI
KI-Client for the Quidditch Game.

## Getting started
You can choose between using Docker or manually installing all dependencies.
Docker is the preferred method as it already installs the toolchain
and all dependencies.

### Docker
In the root directory of the project build the docker image
("ki" is the name of the container, this can be replaced by a
different name):
```
docker build -t ki .
```

Now start the container, you need to map the internal port
(8080 by default, to some external port 80 in this case) and
map the external file (match.json) to an internal file:
```
docker run -v $(pwd)/match.json:match.json -p 80:8080 ki ./KI -a ADRESS -t teamConfig.json -l LOBBY
```
That's it you should now have a running docker instance.

### Manually installing the KI-Client
If you need to debug the ki it can be easier to do this outside
of docker.

### Prerequisites
 * A C++17 compatible Compiler (e.g. GCC-8)
 * CMake (min 3.10) and GNU-Make
 * Adress-Sanitizer for run time checks
 * [SopraNetwork](https://github.com/SoPra-Team-10/Network)
 * [SopraGameLogic](https://github.com/SoPra-Team-10/GameLogic)
 * [SopraMessages](https://github.com/SoPra-Team-10/Messages)
 * Either a POSIX-Compliant OS or Cygwin (to use pthreads)
 * Optional: Google Tests and Google Mock for Unit-Tests

### Compiling the Application
In the root directory of the project create a new directory
(in this example it will be called build), change in this directory.

Next generate a makefile using cmake:
```
cmake ..
```
if any error occurs recheck the prerequisites. Next compile the program:
```
make
```
you can now run the ki by executing the created `KI` file:
```
./KI
```

## Log-Levels

| Log-Level | Color | Explanation |
| ----- | ----- | ---- |
| 0 | - | No log messages |
| 1 | Red | Only error messages |
| 2 | Yellow | Error messages and warning |
| 3 | Blue | Error messages, warning and info messages |
| 4 | White | All messages (error, warning, info and debug) |

## External Librarys
 * [SopraNetwork](https://github.com/SoPra-Team-10/Network)
 * [SopraGameLogic](https://github.com/SoPra-Team-10/GameLogic)
 * [nlohmann::json](https://github.com/nlohmann/json)

## Doxygen Dokumentation
- [Master Branch](https://sopra-team-10.github.io/KI/master/html/index.html)
- [Develop Branch](https://sopra-team-10.github.io/KI/develop/html/index.html)
