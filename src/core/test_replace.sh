#!/bin/bash

definition="constexpr static const"

echo "$definition"

definition=${definition// const/}
echo ">$definition"

exit 0
