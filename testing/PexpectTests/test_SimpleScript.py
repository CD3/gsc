import pexpect
import pytest

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
