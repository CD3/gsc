# # this script demonstrates how control commands are used
# # to change the behaviour of the session runner.
# interactive off
# simulate_typing off
echo "interactive mode is off. delays are off"
# simulate_typing on
echo "interactive mode is off. delays are on"
echo "a three second delay is coming"
# delay 30
echo "delay is over"
# delay 10 # pause for a second before turning off stdout in case the shell has some output
# stdout off
echo "this file was created silently." >> TMP.txt
# stdout on
echo "turning interactive mode on. press enter."
# interactive on
echo "interactive mode is on. delays are on. press enter"
# simulate_typing off
echo "interactive mode is on. delays are off. press enter"
echo "entering pass through mode. after pressing enter, you will be able to interact with the shell. press Ctrl-D and Enter when done."
# passthrough
echo "running session commands again."
# this is a comment
echo "done with commands. will now exit."
