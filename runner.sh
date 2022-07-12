#!/bin/bash

MODULE="memory"
(cd ./$MODULE && ./$MODULE  >$MODULE.log) &

MODULE="cpu"
(cd ./$MODULE && ./$MODULE >$MODULE.log) &

MODULE="kernel"
(cd ./$MODULE && ./$MODULE  >$MODULE.log) &