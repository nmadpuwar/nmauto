#!/bin/bash

# Delete elastic search
kubectl delete -f es-dep.yaml -n airbus

# Delete kibana
kubectl delete -f kibana-dep.yaml -n airbus

# Delete logstash
kubectl delete -f logstash-dep.yaml -n airbus

# Delete configuration's
kubectl delete configMap logstash-config
