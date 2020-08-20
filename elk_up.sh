#!/bin/bash

# Initialize configuration's
kubectl create configmap logstash-config --from-file=logstash/pipeline/logstash.conf

# Start elastic search
kubectl apply -f es-dep.yaml -n airbus

# Start kibana
kubectl apply -f kibana-dep.yaml -n airbus

# Start logstash
kubectl apply -f logstash-dep.yaml -n airbus
