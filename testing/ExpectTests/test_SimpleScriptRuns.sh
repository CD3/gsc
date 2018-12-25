cat > test-gsc.sh << EOF
ls
EOF

cat > test-expect.sh << EOF
set timeout 10
spawn ./gsc test.sh --shell bash
expect -re ".*"
sleep 1
send "a"
expect "l"
send "a"
expect "s"
send "a"
expect ""
send "a"
expect ""
send "\r"
expect ""
sleep 1
EOF

expect test-expect.sh

