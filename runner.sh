#!/bin/bash

MODULE="memory"
(cd ./$MODULE && ./$MODULE >./cfg/$MODULE.log) &

MODULE="cpu"
(cd ./$MODULE && ./$MODULE >./cfg/$MODULE.log) &

MODULE="kernel"
(cd ./$MODULE && ./$MODULE >./cfg/$MODULE.log) &