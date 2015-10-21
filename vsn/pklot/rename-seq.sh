#!/bin/bash

a=0
for i in *.jpg; do
  new_cam1=$(printf "camera11_%04d.jpg" "$a")
  new_cam2=$(printf "camera12_%04d.jpg" "$a")
  mv -- "$i" "$new_cam1"
  ln -s "$new_cam1" "$new_cam2"
  let a=a+1
done
