import pexpect
import pytest
import time

def print_output(proc):
  proc.expect(".")
  print(proc.before)
  print(proc.after)

def test_SimpleScriptInsertMode():
  with open("script.sh", "w") as f:
    f.write("echo hi\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  # the prompt will be printed 3 times:
  # once when the setup command is echo'ed by the shell
  # once when the new prompt is printed by the shell
  # and one more time because the setup-command is loaded before the shell takes over
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")

  assert child.send("b") == 1
  assert child.expect("e") == 0

  assert child.send("b") == 1
  assert child.expect("c") == 0

  assert child.send("b") == 1
  assert child.expect("h") == 0

  assert child.send("b") == 1
  assert child.expect("o") == 0

  assert child.send("b") == 1
  assert child.expect(" ") == 0

  assert child.send("b") == 1
  assert child.expect("h") == 0

  assert child.send("b") == 1
  assert child.expect("i") == 0

  assert child.send("b") == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)

  child.sendcontrol(r"m")
  assert child.expect("\r\n") == 0

  assert child.expect("hi\r\n") == 0

  child.expect(r"\$>>> ")

  child.sendcontrol(r"m")
  assert child.expect("\r\n") == 0
  assert child.expect("\r\n") == 0
  assert child.expect("Session Finished. Press Enter.") == 0
  assert child.expect("\r\n") == 0
  assert child.expect(b"\x00") == 0 # NOT SURE WHAT IS SENDING THIS.

  child.sendcontrol(r"m")
  child.expect(pexpect.EOF)

  assert not child.isalive()

  child.close()
  assert child.exitstatus == 0



def test_BackspaceInInsertMode():

  with open("script.sh", "w") as f:
    f.write("echo hi\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")

  assert child.send("b") == 1
  assert child.expect("e") == 0

  assert child.send("b") == 1
  assert child.expect("c") == 0

  assert child.send(b'\x7f') == 1
  assert child.expect(b'\x08') == 0

  assert child.send("b") == 1
  assert child.expect("c") == 0

  assert child.send("b") == 1
  assert child.expect("h") == 0

  assert child.send("b") == 1
  assert child.expect("o") == 0

  assert child.send("b") == 1
  assert child.expect(" ") == 0

  assert child.send("b") == 1
  assert child.expect("h") == 0

  assert child.send("b") == 1
  assert child.expect("i") == 0

  assert child.send(b'\x7f') == 1
  assert child.expect(b'\x08') == 0

  assert child.send(b'\x7f') == 1
  assert child.expect(b'\x08') == 0

  assert child.send("b") == 1
  assert child.expect("h") == 0

  assert child.send("b") == 1
  assert child.expect("i") == 0


  child.terminate()

def test_CommentsAreIgnored():

  with open("script.sh", "w") as f:
    f.write("# C: comment\n");
    f.write("echo\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")

  assert child.send("bbbb") == 4
  assert child.expect("echo") == 0



  child.terminate()



  with open("script.sh", "w") as f:
    f.write("# C : comment 1\n");
    f.write("#C: comment 2\n");
    f.write(" # COMMENT : comment 3\n");
    f.write(" #COMMENT: comment 3\n");
    f.write("echo\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")

  assert child.send("bbbbbbbbbbb") == 11
  assert child.expect("echo") == 0



  child.terminate()

def test_CommandModeQuit():
  with open("script.sh", "w") as f:
    f.write("echo\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  assert child.send("") == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)
  assert child.send("b") == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)


  assert child.isalive()

  assert child.send("q") == 1
  time.sleep(1)

  assert not child.isalive()

  child.close()
  assert child.exitstatus == 1

def test_CommandModeQuit():
  with open("script.sh", "w") as f:
    f.write("echo\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)

  assert child.isalive()

  child.send("")
  time.sleep(1)

  assert not child.isalive()

  child.close()
  assert child.exitstatus == 1

def test_SilenceOutput():

  with open("script.sh", "w") as f:
    f.write("ls -l\n");
    f.write("echo hi\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")

  assert child.send("") == 1
  assert child.send("s") == 1
  assert child.send("i") == 1

  assert child.send("b") == 1
  assert child.send("b") == 1

  assert child.send("") == 1
  assert child.send("v") == 1
  assert child.send("i") == 1

  assert child.send("bbb") == 3
  assert child.expect(" -l") == 0

  assert child.send("") == 1
  assert child.send("s") == 1
  assert child.send("i") == 1

  assert child.sendcontrol('m') == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)

  assert child.send("bbbb bb") == 7
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)

  assert child.send("") == 1
  assert child.send("v") == 1
  assert child.send("i") == 1

  assert child.sendcontrol('m') == 1
  assert child.expect("^\r\nhi") == 0

def test_ForwardBackward():
  with open("script.sh", "w") as f:
    f.write("echo 1 \n");
    f.write("echo 2 \n");
    f.write("echo 3 \n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")

  # skip first line
  child.send("")
  child.send("j")
  child.send("i")

  child.send("bbbbbb")
  assert child.expect("echo 2") == 0

  # go back to fist line
  child.send("")
  child.send("kk")
  child.send("i")

  child.send("bbbbbb")
  assert child.expect("echo 1") == 0

  # skip to end
  child.send("")
  child.send("jjjjjj")
  child.send("i")

  child.send("bbbbbb")
  assert child.expect("echo 3") == 0

  # go back to begining
  child.send("")
  child.send("kkk")
  child.send("i")

  child.send("bbbbbb")
  assert child.expect("echo 1") == 0

def test_KeyBindings():
  with open("script.sh", "w") as f:
    f.write("echo 1 \n");
    f.write("echo 2 \n");
    f.write("echo 3 \n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "' --key-binding='120:CommandMode_Quit' """,timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")

  assert child.send("") == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)

  assert child.isalive()

  assert child.send("x") == 1
  time.sleep(1)

  assert not child.isalive()

def test_ContextVariables():
  with open("script.sh", "w") as f:
    f.write("echo %msg% \n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "' --context-variable="'msg'='hello!'"' """,timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")

  child.send("bbbbbb")
  assert child.expect("echo ") == 0

  child.send("bbbbbbb")
  assert child.expect("hello! ") == 0

def test_ExitCommand():
  with open("script.sh", "w") as f:
    f.write("echo\n");
    f.write("# EXIT\n");
    f.write("echo\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.send("bbbbbb")
  assert child.isalive()
  assert child.expect("echo") == 0

  child.send("\r")
  time.sleep(1)

  assert not child.isalive()

  child.close()
  assert child.exitstatus == 0


def test_SkipCommand():
  with open("script.sh", "w") as f:
    f.write("echo 1\n");
    f.write("# SKIP\n");
    f.write("echo 2\n");
    f.write("# RESUME\n");
    f.write("echo 3\n");

  child = pexpect.spawn("""./gsc script.sh --shell bash --setup-command='PS1="$>>> "'""",timeout=2)
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.expect(r"\$>>> ")
  child.send("bbbbbbbb")
  assert child.expect("echo 1") == 0
  child.sendcontrol(r"m")
  assert child.expect("\r\n") == 0
  child.send("bbbbbbbb")
  assert child.expect("echo 3") == 0


