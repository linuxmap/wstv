var J = (function () {
  var isSafari = /Apple/.test(navigator.vendor) && $.browser.safari;
  var isIE = $.browser.msie && $.browser.version >= 8;

  if (isIE) {
    var _JSON_stringify = JSON.stringify;
    JSON.stringify = function (obj) {
      eval("var j = '" + _JSON_stringify(obj) + "';");
      return j;
    };
  }

  (function($){
    $.fn.slider =function (callback, options) {
      var options = options || {};
      this.each(function (){
        var isMove=false;
        var _this = $(this).find('.slider-bar');
        var _thisWidth = _this.width();
        var cursor =$(this).find('.cursor');
        var cursorWidth = cursor.width();

        var up = $(this).find('.up');
        var down = $(this).find('.down');

        var dx, dy;

        var This = this;
        if (options.initP) {
          var left = options.initP * (_thisWidth - cursorWidth);
          cursor.css({left: left});
          if (callback) callback.call(this, options.initP);

          if (options.reset$) {
            options.reset$.click(function (e) {
              var left = options.initP * (_thisWidth - cursorWidth);
              cursor.css({left: left});
              if (callback) callback.call(This, options.initP);
            });
          }

        }

        var updown = function (event, isUp) {
          event.preventDefault();
          var x = cursor.offset().left - _this.offset().left;
          var p = x / (_thisWidth - cursorWidth);
          if (isUp) {
            p += 0.02;
            if (p > 1) p = 1;
          } else {
            p -= 0.02;
            if (p < 0) p = 0;
          }
          x = p * (_thisWidth - cursorWidth);

          cursor.css({left: x});
          if (callback) callback.call(This, p);
        };

        up.click(function (event) {
          updown(event, true);
        });

        down.click(function (event) {
          updown(event, false);
        });

        _this.mousemove(function(event){
          event.preventDefault();
          if(isMove){
            var eX = event.pageX;
            var d = eX - dx;
            if (d < 0 || d + cursorWidth > _thisWidth) return false;

            var p = d / (_thisWidth - cursorWidth);
            if (p > 1) p = 1;
            if (p < 0) p = 0;
            x = p * (_thisWidth - cursorWidth);

            cursor.css({left: x});
            if (callback) callback.call(This, p);
          }
        }).click(function (event, p) {
          if (!$.isNumeric(p)) {
            var x = event.pageX - _this.offset().left;
            p = x / (_thisWidth - cursorWidth);
          }

          if (p > 1) p = 1;
          if (p < 0) p = 0;
          x = p * (_thisWidth - cursorWidth);

          cursor.css({left: x});
          if (callback) callback.call(This, p);
        });;

        $(document).mouseup(function (){
          isMove=false;
        }).mousemove(function (event) {
          if (isMove) {
            event.preventDefault();
            return false;
          }
        });

        cursor.mousedown(function(event){
          if($(event.target).is(cursor)){
            isMove = true;
            dx=event.pageX - parseInt(cursor.css("left"));
          }
        });
      });
    };
  })(jQuery);

  var initI18n = function (callback) {
    var lang = cookie.get('lang');
    if (lang.search('_') === -1) {
      lang = $.i18n.browserLang();
      cookie.set('lang', lang, 31536000)
    }
    $('html').attr('lang', lang.split('_').shift());

    $.i18n.properties({
      name: 'messages',
      path: '/i18n/',
      mode: 'map',
      language: lang,
      callback: function () {
        if (callback) callback();
      }
    });
  };

  var cookie = {
    set: function (name, value, lifetime) {
      var str = name + '=' + value + ';';
      if (lifetime) {
        var d = new Date();
        d.setTime(d.getTime() + lifetime * 1000);
        str += 'expires=' + d.toGMTString() + ';';
      }
      str += 'path=/;';
      document.cookie = str;
    },
    get: function (name) {
      var cookies = document.cookie;
      var i = cookies.search(name + '=');
      // set empty value on IE8 will not work
      if (i === -1) return '';

      cookies = cookies.substr(i);
      i = cookies.search(';');
      if (i >= 0) cookies = cookies.substr(0, i);

      return cookies.split('=').pop();
    }
  };

  var playRTSP = function (url, channel, callback) {
    var channel = channel || 1;

    url += '?user=' + J.cookie.get('username')
    + '&passwd=' + J.cookie.get('password');

    var html = ''
    + '<embed '
    + ' name       = "screen"'
    + ' src        = "#"'
    + ' href       = "javaScript:void(0)"'
    + ' qtsrc      = "' + url + '"'
    + ' width      = "100%"'
    + ' height     = "100%"'
    + ' loop       = "false"'
    + ' autoplay   = "true"'
    + ' enablejavascript  = "true"'
    + ' controller = "false"'
    + ' plugin     = "quicktimeplugin"'
    + ' type       = "video/quicktime"'
    + ' scale="tofit"'
    + ' kioskmode="true"'
    + ' target     = "myself"'
    + ' cache      = "false"'
    + ' pluginspage= "http://www.apple.com/quicktime/download/" />';

    var $player = $('#player');
    var $screen = $player.find('.current');
    if (!$screen.length) {
      $screen = $player.find('#screen-' + channel);
    } else {
      $screen.removeClass('current');
    }

    var preVideo = -1;
    if ($screen.data('video') > 0) { // playing
      preVideo = $screen.data('video');
      $('#video-' + preVideo + ' .doing img').trigger('click');
    }

    $screen.html(html);

    // TODO support linux
    if (!('Stop' in $screen.find('embed')[0]) && !$.browser.chrome) {
      var willDownload = confirm($.i18n.prop('download-quicktime'));
      if (willDownload) {
        var lang = J.cookie.get('lang').split('_').pop().toLowerCase();
        if (lang === 'us') {
          lang = ''
        } else {
          lang += '/';
        }
        location.href = 'http://www.apple.com/'
        + lang
        + 'quicktime/download/';
      } else {
        J.cookie.set('username', '', -1000);
        J.cookie.set('password', '', -1000);
        location.reload();
        return;
      }
    }

    if (callback) {
      callback($screen, preVideo);
    }
  };

  var playAllVideo = function (G) {
    var $list = $('#stream-list .list');
    var playAll = function (isMain) {
      G.isPlayAll = true;
      $('#player .screen .current').removeClass('current');
      if (isMain) {
        $list.find('.play.m img').trigger('click');
      } else {
        $list.find('.play.a img').trigger('click');
      }
    };

    var playAllType = J.cookie.get('play-all-type');

    if (playAllType) {
      playAll(playAllType === 'm');
      return;
    }

    var html = ''
    + '<div>'
    + '  <input id="play-dialog-a" type="radio" name="type" value="a" checked><label for="play-dialog-a">' + $.i18n.prop('as') + '</label>'
    + '  <input id="play-dialog-m" type="radio" name="type" value="m"><label for="play-dialog-m">' + $.i18n.prop('ms') + '</label><br>'
    + ' <div class="remember">'
    + '  <input id="play-dialog-r" type="checkbox" name="remember"><label for="play-dialog-r">' + $.i18n.prop('remember') + '</label>'
    + ' </div>'
    + '</div>'
    + '';
    $list.find('.doing').trigger('click');
    J.popWin($.i18n.prop('play-dialog-title'), html, function ($win) {
      $win.find('.ok').click(function () {
        var playType = $win.find('input[name="type"]:checked').val();
        var remember = $win.find('input[name="remember"]:checked').val();
        if (remember) G.playAllType = playType;
        if (remember) J.cookie.set('play-all-type', playType);

        var isMain = playType === 'm';
        playAll(isMain);
      });
    }, {
      className: 'play-dialog'
    });
  };

  var getStreamList = function () {
    var streams = [];
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'yst',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'get_video'
      },
      async: false,
      cache: false
    }).done(function (data) {
      if (data.status != 'ok') return;
      streams = data.data;
      var $list = $('#stream-list .list');
      var html = '';
      for (var i = 0; i < streams.length; i++) {
        var data = '';
        for (var name in streams[i]) {
          data += 'data-' + name + '="' + streams[i][name] + '" ';
        }
        var item = ''
        + '<div class="item" id="video-' + streams[i].id + '" data-channel="' + streams[i].id + '" ' + data + '>'
        + '  <a class="ie record" href="#" data-i18n-l="record" data-i18n-a="title"><img src="/img/icon-record.png" /></a>'
        + '  <a class="play m" href="#" data-i18n-l="ms" data-i18n-a="title"><img src="/img/icon-play-main.png" /></a>'
        + '  <a class="play a" href="#" data-i18n-l="as" data-i18n-a="title"><img src="/img/icon-play-addition.png" /></a>'
        + '  <span data-i18n-l="channel"></span> ' + streams[i].id
        + '</div>';
        html += item;
      }

      $list.html(html);

    });
    return streams.length;
  };

  var getPresetList = function (channel) {
    var presets = [];
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'preset',
        param: JSON.stringify({
          type: 0,
          chnid: channel
        })
      },
      async: false,
      cache: false
    }).done(function (data) {
      if (data.status === 'ok') {
        presets = data.data;
      }
    });

    return presets;
  };

  var delPreset = function (channel, presetId) {
    var result = false;
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'preset',
        param: JSON.stringify({
          type: 2,
          chnid: channel,
          presetid: presetId
        })
      },
      async: false,
      cache: false
    }).done(function (data) {
      if (data.status === 'ok') {
        result = true;
      }
    });

    return result;
  };

  var addPreset = function (channel, presetId, presetName) {
    var result = false;
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'preset',
        param: JSON.stringify({
          type: 1,
          chnid: channel,
          presetid: presetId,
          name: presetName
        })
      },
      async: false,
      cache: false
    }).done(function (data) {
      if (data.status === 'ok') {
        result = true;
      } else {
        result = data.status;
      }
    });

    return result;
  };

  var applyPreset = function (channel, presetId) {
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'preset',
        param: JSON.stringify({
          type: 3,
          chnid: channel,
          presetid: presetId
        })
      },
      async: false,
      cache: false
    })
  };

  var startPatrol = function (channel, patrolId) {
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'patrol',
        param: JSON.stringify({
          type: 3,
          chnid: channel,
          patrolid: patrolId
        })
      },
      async: false,
      cache: false
    })
  };

  var stopPatrol = function (channel, patrolId) {
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'patrol',
        param: JSON.stringify({
          type: 4,
          chnid: channel,
          patrolid: patrolId
        })
      },
      async: false,
      cache: false
    })
  };

  var getPatrolPresets = function (channel, patrolId) {
    var presets = [];
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'patrol',
        param: JSON.stringify({
          type: 0,
          chnid: channel,
          patrolid: patrolId
        })
      },
      async: false,
      cache: false
    }).done(function (response) {
      if (response.status === 'ok') presets = response.data;
    });

    return presets;
  };

  var addPatrolPreset = function (channel, patrolId, presetId, time) {
    var result = false;
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'patrol',
        param: JSON.stringify({
          type: 1,
          chnid: channel,
          patrolid: patrolId,
          presetid: presetId,
          staytime: time
        })
      },
      async: false,
      cache: false
    }).done(function (response) {
      if (response.status === 'ok') {
        result = true;
      } else {
        result = response.status;
      }
    });

    return result;
  };

  var delPatrolPreset = function (channel, patrolId, presetIndex) {
    var result = false;
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'patrol',
        param: JSON.stringify({
          type: 2,
          chnid: channel,
          patrolid: patrolId,
          presetid: presetIndex
        })
      },
      async: false,
      cache: false
    }).done(function (response) {
      if (response.status === 'ok') result = true;
    });

    return result;
  };

  var clearIEOnlyNode = function () {
    $('.ie').each(function () {
      $(this).addClass('hide');
    });
  };

  var hideNoIENode = function () {
    $('.noie').each(function () {
      $(this).addClass('none');
    });
  };

  var clearSafariOnlyNode = function () {
    $('.safari').each(function () {
      $(this).addClass('hide');
    });
  };

  var initScreen = function (num) {
    if (num > 2) {
      var num = Math.ceil(Math.sqrt(num));
    }

    var size = 100 / num;
    var $screen = $('#player .screen');
    var $_screen = $('<div />');

    for (var i = 1; i <= Math.pow(num, 2); i++) {
      var subscreen = $('<div class="subscreen" style="width: ' + size + '%; height: ' + size + '%;"id="screen-' + i + '" />');
      $_screen.append(subscreen);
    }

    $screen.html($_screen.html());
  };

  var getDeviceInfo = function () {
    var port = null;
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'yst',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'get_port'
      },
      async: false,
      cache: false
    }).done(function (info) {
      port = info.data.nPort;
    });

    return {
      port: parseInt(port)
    };
  };

  var popWin = function (title, html, callback, params) {
    params = params || {};

    var className = params.className || ''
    var winHtml = ''
    + '<div class="pop-win ' + className+ '">'
    + '<div class="inner">'
    + '  <div class="title">' + title + '</div>'
    + '  <div class="content">' + html + '</div>'
    + '  <div class="buttons">'
    + '    <button class="ok">' + $.i18n.prop('ok') + '</button>'
    + '    <button class="cancel">' + $.i18n.prop('cancel') + '</button>'
    + '  </div>'
    + '</div>'
    + '</div>';

    var $win = $(winHtml);
    $win.find('.cancel').click(function () {
      $('#player').removeClass('hide');
      $win.remove();
    });

    $win.find('.ok').click(function () {
      $('#player').removeClass('hide');
      $win.remove();
    });

    $('#player').addClass('hide');
    $('body').append($win);
    if (callback) callback($win);
  };

  return {
    initI18n: initI18n,
    getStreamList: getStreamList,
    getPresetList: getPresetList,
    delPreset: delPreset,
    applyPreset: applyPreset,
    addPreset: addPreset,
    startPatrol: startPatrol,
    stopPatrol: stopPatrol,
    addPatrolPreset: addPatrolPreset,
    delPatrolPreset: delPatrolPreset,
    getPatrolPresets: getPatrolPresets,
    initScreen: initScreen,
    playRTSP: playRTSP,
    playAllVideo: playAllVideo,
    clearIEOnlyNode: clearIEOnlyNode,
    hideNoIENode: hideNoIENode,
    clearSafariOnlyNode: clearSafariOnlyNode,
    getDeviceInfo: getDeviceInfo,
    popWin: popWin,
    cookie: cookie
  };
})();
