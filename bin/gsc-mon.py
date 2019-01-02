import urwid
import os, json, time, socket

import pyparsing
from argparse import ArgumentParser





class SFFormatter(object):
  def __init__(self, delimiters=None):
    self.delimiters = ('{','}') if delimiters is None else delimiters
    self.throw = False
    self.warn  = False

    self.token = pyparsing.Literal(self.delimiters[0]) + pyparsing.SkipTo(pyparsing.Literal(self.delimiters[1]), failOn=pyparsing.Literal(self.delimiters[0])) + pyparsing.Literal(self.delimiters[1])
  

  def fmt(self,text,*args,**kwargs):

    def replaceToken(text,loc,toks):
      exp = toks[1]
      try:
        # use the build-in string format method
        return ('{'+exp+'}').format(*args,**kwargs)
      except Exception as e:
        if self.throw:
          raise e
        if self.warn:
          print("WARNING: failed to replace '"+exp+"' using string.format().")

        return None

    self.token.setParseAction(replaceToken)
    text = self.token.transformString( text )

    return text


def fmt( text, *args, **kwargs ):
  delimiters = None
  if 'delimiters' in kwargs:
    delimiters = kwargs['delimiters']

  formatter = SFFormatter(delimiters=delimiters)

  return formatter.fmt( text, *args, **kwargs )



class Namespace(dict):
    def __init__(self,**kwargs):
        dict.__init__(self,kwargs)
        self.__dict__ = self
    def fmt(self,text):
      '''Format a string using self as context.'''
      return fmt(text, **self.__dict__)
    def call(self,func):
      '''Call a function with argument from self.'''
      pass



class TemplateDisplay(urwid.Text):
  """
  A class for displaying template-based text
  """
  def __init__(self,template):
    super().__init__("")
    self.template = template

  def render_text(self,*args,**kwargs):
    text = fmt( self.template, *args, **kwargs )
    self.set_text(text)


def input_handler(key):
    if key in ('q', 'Q'):
        raise urwid.ExitMainLoop()

def file_handler():
  text = monitor.recv(4096)

  try:
    status = json.loads(text)
    input_mode_display.render_text(**status)
    line_status_display.render_text(**status)
  except Exception as e:
    monitor_display.render_text(message="There was an error:\n"+str(e))

  

palette = [
    ('divider','','','','g27','#a06'),
    ('ghost','','','','g27','#a06'),
    ('red','','','','g27','#a06')
    ]




if __name__ == "__main__":

  parser = ArgumentParser(description="A tool to monitor gsc sessions.")
  parser.add_argument("gsc_socket",
                      action="store",
                      default="localhost:3000",
                      help="Unix domain socket that gsc process is writing to." )
  args = parser.parse_args()

  monitor = socket.socket()
  host,port = args.gsc_socket.split(":")
  monitor.connect( (host,int(port)) )










  monitor_display = TemplateDisplay("Messages:\n{message}")

  line_status_display = TemplateDisplay("Previous: {previous line}\nCurrent: {current line}\nNext: {next line}")
  input_mode_display  = TemplateDisplay("Input Mode: {input mode}")


  pile = urwid.Pile([
    urwid.Filler( monitor_display, 'top' ),
    urwid.Filler( urwid.AttrMap(urwid.Divider(),'divider') ),
    urwid.Filler( line_status_display, 'top' ),
    urwid.Filler( input_mode_display, 'bottom' )
    ])

  loop = urwid.MainLoop(pile, palette=palette, unhandled_input=input_handler)
  loop.watch_file(monitor,file_handler)
  loop.run()

  monitor.close()
