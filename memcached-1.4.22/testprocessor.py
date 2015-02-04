# -*- coding: utf-8 -*-
from twisted.web import http, resource, server
class clsAcceptor(object):
  """
  所有请求的父类
  """
#  def __init__(self):

  def set_req_func(self, func_list):
    """
    设置接收到请求时调用的处理函数列表
    """
    self.req_process_func_list = func_list


class clsUcwebServer(clsAcceptor, resource.Resource):
  def __init__(self):
    resource.Resource.__init__(self)
  def render_GET(self, request):
    pass
  def render_POST(self, request):
    #self.req_process_func_list[0](request) -> several worker threads work.
    pass


class FHTTPChannel(http.HTTPChannel):
  def __init__(self):
    http.HTTPChannel.__init__(self)
  def connectionMade(self):
    self.setTimeout(self.timeOut)


class FRequest(server.Request):
  """
  """
  def __init__(self, *args, **kw):
    """
    """
    server.Request.__init__(self, *args, **kw)
    self.arrived_time = None
    self.had_req_dump = False
    self.project_type = 0

class FSite(server.Site):
  """
  """
  requestFactory = FRequest
  def __init__(self, resource, logPath=None, timeout=60*60*12):
    """
    """
    server.Site.__init__(self, resource, logPath, timeout)
    self.protocol = FHTTPChannel
    self.keep_client_connections = []


def ucweb_req_process(http_request):
  pass

def start_state_agent():
  #do sth.
  reactor.callLater(int(config.state_agent_interval), start_state_agent)

from twisted.internet import reactor
ucweb_service_port = [["", 8085]]
ucweb_accept = clsUcwebServer()
ucweb_accept.set_req_func((ucweb_req_process,)) #ucweb_req_processor.put_req(ucweb_req)
for ip, port in ucweb_service_port: #
    site = FSite(ucweb_accept, timeout=120)
    ucweb_service_port = reactor.listenTCP(port, site, interface=ip)
reactor.callLater(int(config.state_agent_interval), start_state_agent)
#reactor.callLater方法用于设置定时事件：
#reactor.callLater函数包含两个必须参数，等待的秒数，和需要调用的函数
reactor.run(installSignalHandlers=0)

