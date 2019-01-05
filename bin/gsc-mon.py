#! /usr/bin/python3
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
    if isinstance( self.template, list):
      text = list()
      for t in self.template:

        if isinstance(t,tuple):
          text.append((t[0],fmt( t[1], *args, **kwargs )))
        else:
          pass
          text.append(fmt( t, *args, **kwargs ))

      self.set_text(text)
    else:
      if isinstance(self.template,tuple):
        text = self.template[0],fmt( self.template[1], *args, **kwargs )
      else:
        text = fmt( self.template, *args, **kwargs )

    self.set_text(text)


def input_handler(key):
    if key in ('q', 'Q'):
        raise urwid.ExitMainLoop()


def file_handler():
  text,addr = monitor.recvfrom(4096)

  try:
    monitor_display.render_text(message="None")
    status = json.loads(text)
    status['current line remainder'] = status['current line'].replace( status['current line progress'], "" )
    input_mode_display.render_text(**status)
    line_status_display.render_text(**status)
  except Exception as e:
    monitor_display.render_text(message="There was an error:\n"+str(e)+"\n"+text.decode('utf-8'))

def poll( loop, data ):
  monitor.sendto(b'update',(host,int(port)))
  loop.set_alarm_in(0.1,poll,None)
  

palette = [
    ('divider','','','','g27','#a06'),
    ('lg','light gray','default'),
    ('dg','dark gray','default'),
    ('dr','dark red','default'),
    ('lr','light red','default'),
    ]




if __name__ == "__main__":

  parser = ArgumentParser(description="A tool to monitor gsc sessions.")
  parser.add_argument("gsc_socket",
                      action="store",
                      default="localhost:3000",
                      help="Unix domain socket that gsc process is writing to." )
  args = parser.parse_args()

  monitor = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  host,port = args.gsc_socket.split(":")










  monitor_display = TemplateDisplay("Messages:\n{message}")

  line_status_display = TemplateDisplay([('dg', "P: {previous line}\n"),
                                         ('dr', "C: {current line progress}"),
                                         ('lr', "{current line remainder}\n"),
                                         ('dg', "N: {next line}\n"),
                                         "\n#: {current line number}/{total number lines}\n"
                                       ])
  input_mode_display  = TemplateDisplay("Input Mode: {input mode}")


  pile = urwid.Pile([
    urwid.Filler( monitor_display, 'top' ),
    urwid.Filler( urwid.AttrMap(urwid.Divider(),'divider') ),
    urwid.Filler( line_status_display, 'top' ),
    urwid.Filler( input_mode_display, 'bottom' )
    ])

  loop = urwid.MainLoop(pile, palette=palette, unhandled_input=input_handler)
  loop.watch_file(monitor,file_handler)
  loop.set_alarm_in(0.1,poll,None)
  
  loop.run()

  monitor.close()
