#!/bin/sh
tmux new-session -d -s web_chrp './build/chrp --mode=web --port=234'
tmux split-window -h 'busybox httpd -p 0.0.0.0:80 -f -h web'
tmux attach -t web_chrp
