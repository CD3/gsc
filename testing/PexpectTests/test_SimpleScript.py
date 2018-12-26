import pexpect
import pytest
import time

def test_SimpleScriptInsertMode():
  with open("script.sh", "w") as f:
    f.write("echo hi\n");

  child = pexpect.spawn("./gsc script.sh --shell bash")
  child.expect(r"\$ ")

  assert child.send("a") == 1
  assert child.expect("e") == 0

  assert child.send("a") == 1
  assert child.expect("c") == 0

  assert child.send("a") == 1
  assert child.expect("h") == 0

  assert child.send("a") == 1
  assert child.expect("o") == 0

  assert child.send("a") == 1
  assert child.expect(" ") == 0

  assert child.send("a") == 1
  assert child.expect("h") == 0

  assert child.send("a") == 1
  assert child.expect("i") == 0

  assert child.send("a") == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)

  child.sendcontrol(r"m")
  assert child.expect("\r\n") == 0

  assert child.expect("hi\r\n") == 0

  child.expect(r"\$ ")

  child.sendcontrol(r"m")
  assert child.expect("\r\n") == 0
  assert child.expect("\r\n") == 0
  assert child.expect("Session Finished. Press Enter.") == 0
  assert child.expect("\r\n") == 0
  assert child.expect(b"\x00") == 0 # NOT SURE WHAT IS SENDING THIS.

  child.sendcontrol(r"m")
  child.expect(pexpect.EOF)

  assert not child.isalive()



def test_BackspaceInInsertMode():

  with open("script.sh", "w") as f:
    f.write("echo hi\n");

  child = pexpect.spawn("./gsc script.sh --shell bash",timeout=1)
  child.expect(r"\$ ")

  assert child.send("a") == 1
  assert child.expect("e") == 0

  assert child.send("a") == 1
  assert child.expect("c") == 0

  assert child.send(b'\x7f') == 1
  assert child.expect(b'\x08') == 0

  assert child.send("a") == 1
  assert child.expect("c") == 0

  assert child.send("a") == 1
  assert child.expect("h") == 0

  assert child.send("a") == 1
  assert child.expect("o") == 0

  assert child.send("a") == 1
  assert child.expect(" ") == 0

  assert child.send("a") == 1
  assert child.expect("h") == 0

  assert child.send("a") == 1
  assert child.expect("i") == 0

  assert child.send(b'\x7f') == 1
  assert child.expect(b'\x08') == 0

  assert child.send(b'\x7f') == 1
  assert child.expect(b'\x08') == 0

  assert child.send("a") == 1
  assert child.expect("h") == 0

  assert child.send("a") == 1
  assert child.expect("i") == 0


  child.terminate()

def test_CommentsAreIgnored():

  with open("script.sh", "w") as f:
    f.write("# comment\n");
    f.write("echo\n");

  child = pexpect.spawn("./gsc script.sh --shell bash",timeout=1)
  child.expect(r"\$ ")

  assert child.send("aaaa") == 4
  assert child.expect("echo") == 0



  child.terminate()



  with open("script.sh", "w") as f:
    f.write("# comment 1\n");
    f.write("#comment 2\n");
    f.write(" # comment 3\n");
    f.write(" #comment 3\n");
    f.write("echo\n");

  child = pexpect.spawn("./gsc script.sh --shell bash",timeout=1)
  child.expect(r"\$ ")

  assert child.send("aaaaaaaaaaa") == 11
  assert child.expect("echo") == 0



  child.terminate()

def test_CommandModeQuit():
  with open("script.sh", "w") as f:
    f.write("echo\n");

  child = pexpect.spawn("./gsc script.sh --shell bash",timeout=1)
  child.expect(r"\$ ")
  assert child.send("") == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)
  assert child.send("a") == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)


  assert child.isalive()

  assert child.send("q") == 1
  time.sleep(1)

  assert not child.isalive()

def test_SilenseOutput():

  with open("script.sh", "w") as f:
    f.write("ls -l\n");
    f.write("echo hi\n");

  child = pexpect.spawn("./gsc script.sh --shell bash",timeout=1)
  child.expect(r"\$ ")

  assert child.send("") == 1
  assert child.send("s") == 1
  assert child.send("i") == 1

  assert child.send("a") == 1
  assert child.send("a") == 1

  assert child.send("") == 1
  assert child.send("v") == 1
  assert child.send("i") == 1

  assert child.send("aaa") == 3
  assert child.expect(" -l") == 0

  assert child.send("") == 1
  assert child.send("s") == 1
  assert child.send("i") == 1

  assert child.sendcontrol('m') == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)

  assert child.send("aaaa aa") == 7
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)

  assert child.send("") == 1
  assert child.send("v") == 1
  assert child.send("i") == 1

  assert child.sendcontrol('m') == 1
  assert child.expect("^\r\nhi") == 0




