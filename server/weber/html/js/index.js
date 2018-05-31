$(document).ready(function () {
  var $loginPanel = $('#login');
  if ($loginPanel.hasClass('none')) return;

  $loginPanel.find('#login-username').focus();
});
$(document).ready(function () {
  var X = null;
  var G = {
    currentDeviceInfo: null,
    streamNum: 0,
    opticsProps: {
      contrast: 0,
      brightness: 1,
      saturation: 2,
      sharpness: 3
    },
    setCurrentDeviceInfo: function (info) {
      this.currentDeviceInfo = info
      var $panel = $('#ptz-panel .addition-panel')

      $.each(G.opticsProps, function (name, type) {
        $.ajax({
          url: '/cgi-bin/jvsweb.cgi',
          data: {
            cmd: 'multimedia',
            action: 'imageget',
            username: J.cookie.get('username'),
            password: J.cookie.get('password'),
            param: JSON.stringify({
              chnid: G.currentDeviceInfo.channel,
              type: type
            })
          },
          cache: false
        }).done(function (data) {
          if (data.status === 'ok') {
            $panel.find('.' + name + ' .slider-bar').trigger('click', data.data.value);
          }
        });
      });

      updatePresetList();
    }
  };

  var updatePresetList = function () {
    var presets = J.getPresetList(G.currentDeviceInfo.channel) || [];
    var options = [];
    $.each(presets, function (i, preset) {
      options.push('<option value="'
                   + preset.id + '">'
                   + preset.id + '-' + preset.name + '</option>');
    });
    $('#preset-list').html(options.join(''));
  };

  var isIE = $.browser.msie && $.browser.version >= 8;
  var isSafari = /Apple/.test(navigator.vendor) && $.browser.safari;

  function i18n($handler) {
    $handler.find('[data-i18n-l]').each(function () {
      var $node = $(this);
      var attr = $node.data('i18n-a');
      var label = $node.data('i18n-l');
      if (!label) return;

      if (!attr) {
        $node.html($.i18n.prop(label));
      } else {
        $node.attr(attr, $.i18n.prop(label));
      }
    });
  }

  J.initI18n(function () {
    i18n($('body'));
    $('#login-language').val(J.cookie.get('lang'));
  });

  if ($.browser.msie && $.browser.version < 8) {
    var flag = confirm($.i18n.prop('update-ie'));
    if (flag) location.href = 'http://ib.adnxs.com/clktrb?id=121748';
  }

  $('#login-language').change(function (e) {
    var lang = $(e.target).val();
    if (lang !== J.cookie.get('lang')) {
      J.cookie.set('lang', lang, 3153600);
      $('html').attr('lang', lang.split('_').shift());
      J.initI18n(function () {
        i18n($('body'));
      });
    }
  });

  function init() {
    $('#player').removeClass('hide');

    G.streamNum = J.getStreamList();

    var $list = $('#stream-list .list');
    i18n($list);

    $list.find('.item').click(function (event) {
      var target = $(event.target);
      if (!target.hasClass('current')) {
        $list.find('.item.current').removeClass('current');
        $(this).addClass('current');
        G.setCurrentDeviceInfo($(this).data());
      }
    });

    if (!isIE) {
      J.clearIEOnlyNode();
      if (!isSafari) {
        J.clearSafariOnlyNode();
      }
    } else {
      J.hideNoIENode();
    }

    // active Plugin
    var _X = document.getElementById('XPLAYER');

    if ('JSDivideWindow' in _X) {
      X = _X;
      var info = J.getDeviceInfo();

      X.JSSetDeviceInfo(
        location.hostname,
        J.cookie.get('username'),
        J.cookie.get('password'),
        info.port,
        G.streamNum
      );
      X.JSSetLanguage(J.cookie.get('lang') || 'zh_CN');

      if (!X.attachEvent) X.attachEvent = X.addEventListener;

      X.attachEvent("OnConnectChanged", function (channel, isMain, state) {
        var type = 'a';
        if (isMain) type = 'm';
        if (state != 2) { // disconnect successfully
          $('#video-' + channel + ' .play.' + type + '.doing img').each(function (i, img) {
            img = $(img);
            img.attr('src', img.attr('src').replace('-dark.png', '.png'));
            img.parent().removeClass('doing').parent().removeClass('current');
            G.currentDeviceInfo = null;
          });
        }
      });

      X.attachEvent("OnChnSelected", function (channel, state) {
        var ChannelState = {
          CHS_NOUSE: 0,
          CHS_PREVIWING: 1,
          CHS_REMOTING: 2,
          CHS_LOCALPLAYING: 4,
          CHS_RECORDING: 8,
          CHS_ALARMING: 16,
          CHS_SOUNDING: 32,
          CHS_TALKING: 64,
          CHS_ENABLEALARM: 128
        };
        var state = X.JSGetChannelState(channel);

        if (state & ChannelState.CHS_PREVIWING) { // playing video
          G.setCurrentDeviceInfo($('#video-' + channel).data());
        }

        $list.find('.current').removeClass('current');
        $('#video-' + channel).addClass('current');

        var $img = $('#player .panel .sound img');
        if (state & ChannelState.CHS_SOUNDING) { // playing sound
          if ($img.attr('src').search('-dark') === -1) {
            $img.attr('src', $img.attr('src').replace('.png', '-dark.png'));
          }
        } else {
          if ($img.attr('src').search('-dark') !== -1) {
            $img.attr('src', $img.attr('src').replace('-dark.png', '.png'));
          }
        }

        var $img = $('#player .panel .talk img');
        if (state & ChannelState.CHS_TALKING) { // talking
          if ($img.attr('src').search('-dark') === -1) {
            $img.attr('src', $img.attr('src').replace('.png', '-dark.png'));
          }
        } else {
          if ($img.attr('src').search('-dark') !== -1) {
            $img.attr('src', $img.attr('src').replace('-dark.png', '.png'));
          }
        }
      });
    } else { // non ie
      $('#player .screen').html('');
      J.initScreen(G.streamNum);
      $('#player .screen').click(function (e) {
        var $this = $(this);
        var $target = $(e.target);
        if ($target.hasClass('current')) return;

        $this.find('.current').removeClass('current');
        if ($target.hasClass('subscreen')) {
          $target.addClass('current');
        } else { // click plugin on mac
          $target = $target.parent();

          $target.addClass('current');
          var videoId = $target.data('video');
          if (videoId >= 0) { // click a playing subscreen
            G.setCurrentDeviceInfo($('#video-' + videoId).data());
          }
        }
      });
    }
    /**
     * use setTimeout to call playAllVideo
     * after click callbacks have been registered for video list
     */
    setTimeout(function () {
      J.playAllVideo(G);
    }, 1);
  }

  if (J.cookie.get('username') && J.cookie.get('password')) {
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'account',
        action: 'check',
        param: JSON.stringify({
          acID: J.cookie.get('username'),
          acPW: J.cookie.get('password')
        })
      },
      async: false,
      cache: false
    }).done(function (data) {
      if (data.status !== 'ok') {
        var timeout = 0;
        J.cookie.set('username', '', -100);
        J.cookie.set('password', '', -100);
      } else if (data.status === 'ok') {
        $('#login').addClass('none');
        init();
      }
    });
  } else {
    $('#login-submit').click(function (e) {
      e.preventDefault();
      var $username = $('#login-username');
      var $password = $('#login-password');
      var $rememberme = $('#login-rememberme');

      if (!$username.val()) {
        $username.focus();
        return;
      }

      var username = $username.val();
      var password = $password.val();

      $.ajax({
        url: '/cgi-bin/jvsweb.cgi',
        data: {
          cmd: 'account',
          action: 'check',
          param: JSON.stringify({
            acID: username,
            acPW: password
          })
        },
        async: false,
        cache: false
      }).done(function (data) {
        if (data.status === 'ok') {
          $('#login').addClass('none');
          var timeout = 0;
          if ($rememberme[0].checked) timeout = 31536000; // one year
          J.cookie.set('username', username, timeout);
          J.cookie.set('password', password, timeout);
          $password.val('');

          init();
        } else if (data.status === 'account not exist' || data.status === 'error passwd') {
          alert($.i18n.prop('invalid-user-or-pass'));
        }
      });
    });
  }

  if (J.cookie.get('wide')) {
    $('#container .inner').removeClass('n80');
  }

  $('#stream-list .list').click(function (e) {
    e.preventDefault();
    var $this = $(this);

    var a = $(e.target).parent();
    var $icon = a.find('img');
    var $item = a.parent();
    var info = $item.data();

    if (a.hasClass('play')) {
      if (a.hasClass('doing')) { // stop play
        if (X) {
          $item.find('.record.doing img').trigger('click');
          X.JSDisconnect(info.channel);
        } else {
          $('#player .screen #' + info.screen).html('');
        }

        a.removeClass('doing');
        $icon.attr('src', $icon.attr('src').replace('-dark.png', '.png'));
        if (G.currentDeviceInfo === info) {
          G.currentDeviceInfo = null;
        }
      } else { // start play
        if (!G.isPlayAll) G.setCurrentDeviceInfo(info);

        var useMain = false;
        if (a.hasClass('m')) { // main stream
          var next = a.next();
          if (next.hasClass('doing')) next.find('img').trigger('click');
          var stream = info.stream0;
          useMain = true;
        } else {
          var prev = a.prev();
          if (prev.hasClass('doing')) prev.find('img').trigger('click');
          var stream = info.stream1;
        }

        if (X) {
          if (G.isPlayAll) {
            X.JSConnect(info.channel, info.channel, useMain);
          } else {
            X.JSConnect(info.channel, false, useMain);
          }
        } else {
          J.playRTSP(stream, info.channel, function ($screen, preVideo) {
            $item.data('screen', $screen.attr('id'));
            $screen.data('video', info.channel);
          });
        }

        a.addClass('doing');
        $icon.attr('src', $icon.attr('src').replace('.png', '-dark.png'));
      }
      return;
    }

    if (a.hasClass('record')) {
      if (a.hasClass('doing')) { // stop play
        if (X) {
          if (X.JSRecordStop(info.channel)) {
            a.removeClass('doing');
            $icon.attr('src', $icon.attr('src').replace('-dark.png', '.png'));
          }
        }
      } else { // start play
        var $p1 = a.next();
        var $p2 = $p1.next();
        if ($p1.hasClass('doing') || $p2.hasClass('doing')) {
          if (X) {
            if (X.JSRecordStart(info.channel)) {
              a.addClass('doing');
              $icon.attr('src', $icon.attr('src').replace('.png', '-dark.png'));
            }
          }
        }
      }
      return;
    }
  });

  $('.slider').slider(function (p) {
    if (!G.currentDeviceInfo) return;

    var title = $(this).prev().data('i18n-l');

    if (!(title in G.opticsProps)) return;

    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'multimedia',
        action: 'imageset',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        param: JSON.stringify({
          chnid: G.currentDeviceInfo.channel,
          type: G.opticsProps[title],
          value: p
        })
      },
      async: false,
      cache: false
    });
  }, {
    reset$: $('#ptz-panel .addition-panel .reset'),
    initP: 0.5
  });

  $('.hover img[src*=icon]').mouseover(function () {
    var $this = $(this);

    var icon = $this.attr('src');
    var darkIcon = icon.split('.').shift() + '-dark.png';
    $this.attr('src', darkIcon);
    $this.mouseout(function () {
      $this.attr('src', icon);
    });
  });

  $('#header .menu-bar').click(function (e) {
    var $item = $(e.target);
    var label = $item.data('i18n-l');

    if (label === 'playback') {
      if (X) {
        X.JSPlayBack();
      }

      return;
    }

    if (label === 'log') {
      if (X) {
        X.JSLogInfo();
      }

      return;
    }

    if (label === 'device-conf') {
      if (X) {
        X.JSRemoteConfig(0);
      }

      return;
    }

    if (label === 'client-conf') {
      if (X) {
        X.JSLocalConfig();
      }

      return;
    }

    if (label === 'version') {
      if (!isIE) $('#player').addClass('hide');
      alert($.i18n.prop('version') + ' 0.9');
      $('#player').removeClass('hide');
    }

    if (label === 'logout') {
      J.cookie.set('username', 0, -1);
      J.cookie.set('password', 0, -1);
      location.reload();
    }
  });

  $('#player .panel a').click(function (e) {
    var $this = $(this);
    var label = $this.data('i18n-l');
    var $list = $('#stream-list .list');

    if (label === 'full-screen') {
      if (X) {
        X.JSTvMode();
      } else {
        var $currentScreen = $('#player .screen .current');
        if ($currentScreen.length !== 1) return;

        $currentScreen.addClass('full');
      }
      return;
    }

    if (label === 'four-screen') {
      if (X) {
        X.JSDivideWindow(4);
      }
      return;
    }

    if (label === 'multi-screen') {
      if (X) {
        X.JSDivideWindow(G.streamNum || 1);
      } else {
        var $currentScreen = $('#player .screen .current');
        if ($currentScreen.length !== 1) return;

        $currentScreen.removeClass('full');
      }
      return;
    }

    if (label === 'all-play') {
      // stop all before play
      $this.next().trigger('click');
      J.playAllVideo(G);

      return;
    }

    if (label === 'all-stop') {
      $list.find('.play.doing img').trigger('click');
      G.currentDeviceInfo = null;
      $list.find('.current').removeClass('current');

      G.isPlayAll = false;
      return;
    }

    if (label === 'all-record') {
      $list.find('.record img').trigger('click');
      return;
    }

    if (label === 'all-stop-record') {
      $list.find('.record.doing img').trigger('click');

      return;
    }

    if (label === 'screenshot') {
      if (X) {
        X.JSVideoCapture(0);
      }
      return;
    }

    if (label === 'sound') {
      if (!G.currentDeviceInfo) return;

      if (X) {
        var state = X.JSGetChannelState(G.currentDeviceInfo.channel);
        var $img = $this.find('img');
        if (state & 32) {
          if (X.JSCloseAudio(G.currentDeviceInfo.channel)) {
            if ($img.attr('src').search('-dark') !== -1) {
              $img.attr('src', $img.attr('src').replace('-dark.png', '.png'));
            }
          }
        } else {
          if (X.JSOpenAudio(G.currentDeviceInfo.channel)) {
            if ($img.attr('src').search('-dark') === -1) {
              $img.attr('src', $img.attr('src').replace('.png', '-dark.png'));
            }
          }
        }
      }

      return;
    }

    if (label === 'talk') {
      if (!G.currentDeviceInfo) return;

      if (X) {
        var state = X.JSGetChannelState(G.currentDeviceInfo.channel);
        var $img = $this.find('img');
        if (state & 64) {
          if (X.JSTalkBackStop(G.currentDeviceInfo.channel)) {
            if ($img.attr('src').search('-dark') !== -1) {
              $img.attr('src', $img.attr('src').replace('-dark.png', '.png'));
            }
          }
        } else {
          if (X.JSTalkBackStart(G.currentDeviceInfo.channel)) {
            if ($img.attr('src').search('-dark') === -1) {
              $img.attr('src', $img.attr('src').replace('.png', '-dark.png'));
            }
          }
        }
      }

      return;
    }
  });

  $('#ptz-panel .direction').mousedown(function (e) { // start move
    e.preventDefault();
    if (!G.currentDeviceInfo) return;

    var speed = $(this).parent().find('.speed select').val();
    speed = parseFloat(speed) / 10;
    if (speed < 0 || speed > 1) speed = 0.5;

    var target = $(e.target);

    if (target.hasClass('c')) {
      $.ajax({
        url: '/cgi-bin/jvsweb.cgi',
        data: {
          cmd: 'ptz',
          action: 'move_auto',
          username: J.cookie.get('username'),
          password: J.cookie.get('password'),
          param: JSON.stringify({
            chnid: G.currentDeviceInfo.channel,
            s: speed
          })
        },
        async: false,
        cache: false
      });
      return;
    }

    var x = 0, y = 0;
    if (target.hasClass('n')) {
      y = speed;
    } else if (target.hasClass('en')) {
      x = speed; y = speed;
    } else if (target.hasClass('e')) {
      x = speed;
    } else if (target.hasClass('es')) {
      x = speed; y = -speed;
    } else if (target.hasClass('s')) {
      y = -speed;
    } else if (target.hasClass('ws')) {
      x = -speed; y = -speed;
    } else if (target.hasClass('w')) {
      x = -speed;
    } else if (target.hasClass('wn')) {
      x = -speed; y = speed;
    }
    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        action: 'move',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        param: JSON.stringify({
          chnid: G.currentDeviceInfo.channel,
          x: x,
          y: y
        })
      },
      async: false,
      cache: false
    });
  }).mouseup(function (e) { // stop move
    e.preventDefault();
    if (!G.currentDeviceInfo) return;

    var target = $(e.target);

    if (target.hasClass('c')) {
      $.ajax({
        url: '/cgi-bin/jvsweb.cgi',
        data: {
          cmd: 'ptz',
          action: 'move_auto',
          username: J.cookie.get('username'),
          password: J.cookie.get('password'),
          param: JSON.stringify({
            chnid: G.currentDeviceInfo.channel,
            s: 0
          })
        },
        async: false,
        cache: false
      });
      return;
    }

    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        action: 'move',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        param: JSON.stringify({
          chnid: G.currentDeviceInfo.channel,
          x: 0,
          y: 0
        })
      },
      async: false,
      cache: false
    });
  });

  $('#ptz-panel .optics').mousedown(function (e) {
    e.preventDefault();
    if (!G.currentDeviceInfo) return;

    var $action = $(e.target).parent();
    var $item = $action.parent();

    var type, value = -1;

    if ($item.hasClass('aperture')) { // 光圈
      type = 0;
      if ($action.hasClass('i')) value = 1;
    } else if ($item.hasClass('focus')) { // 焦距
      type = 1;
      if ($action.hasClass('i')) value = 1;
    } else if ($item.hasClass('magnification')) { // 倍率
      type = 2;
      if ($action.hasClass('i')) value = 1;
    }

    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        action: 'lens',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        param: JSON.stringify({
          chnid: G.currentDeviceInfo.channel,
          type: type,
          value: value
        })
      },
      async: false,
      cache: false
    });
  }).mouseup(function (e) {
    e.preventDefault();
    if (!G.currentDeviceInfo) return;

    var $action = $(e.target).parent();
    var $item = $action.parent();

    var type;

    if ($item.hasClass('aperture')) { // 光圈
      type = 0;
    } else if ($item.hasClass('focus')) { // 焦距
      type = 1;
    } else if ($item.hasClass('magnification')) { // 倍率
      type = 2;
    }

    $.ajax({
      url: '/cgi-bin/jvsweb.cgi',
      data: {
        cmd: 'ptz',
        username: J.cookie.get('username'),
        password: J.cookie.get('password'),
        action: 'lens',
        param: JSON.stringify({
          chnid: G.currentDeviceInfo.channel,
          type: type,
          value: 0
        })
      },
      async: false,
      cache: false
    });
  });

  $('#ptz-panel .addition-panel .switch span').click(function (e) {
    $('#ptz-panel .addition-panel .switch img').trigger('click');
  });

  $('#ptz-panel .addition-panel .switch img').click(function (e) {
    var $this = $(this);
    var src = $this.attr('src');
    var $root = $this.parent().parent();
    var panel1 = $root.find('.panel1');
    var panel2 = $root.find('.panel2');
    if (src.search('bg1') !== -1) {
      src = src.replace('1', '2');
    } else {
      src = src.replace('2', '1');
    }

    panel1.toggleClass('none');
    panel2.toggleClass('none');

    $this.attr('src', src);
  });

  $('#ptz-panel .addition-panel .panel2').click(function (e) {
    var $target = $(e.target);

    if ($target.hasClass('reboot')) {
      $('#player').addClass('hide');
      if (!confirm($.i18n.prop('reboot-msg'))) {
        $('#player').removeClass('hide');
        return;
      }

      $.ajax({
        url: '/cgi-bin/jvsweb.cgi',
        data: {
          cmd: 'system',
          action: 'reboot',
          username: J.cookie.get('username'),
          password: J.cookie.get('password')
        },
        async: false,
        cache: false
      }).done(function () {
        var progress = '.';
        var $body = $('body');
        var ping = function () {
          $body.html(progress);
          $.ajax({
            url: '/ping.html',
            cache: false,
            timeout: 1000,
            statusCode: {
              200: function () {
                location.reload();
              }
            }
          });
          progress += '.';
          if (progress.length > 6) progress = '.';
        };

        $body.html('').css({
          background: '#424041',
          color: 'white',
          'font-size': '10em',
          'text-align': 'center'
        });

        setInterval(ping, 1000);
      });

      return;
    }

    if ($target.hasClass('color-conf')) {
      return;
    }

    if ($target.hasClass('change-ratio')) {
      var isWide = J.cookie.get('wide');
      if (!isWide) {
        $('#container .inner').removeClass('n80');
        J.cookie.set('wide', 1, 31536000);
      } else {
        $('#container .inner').addClass('n80');
        J.cookie.set('wide', 1, -1);
      }
      return;
    }
  });

  $('#ptz-panel .preset').click(function (event) {
    var $a = $(event.target).parent();
    if (!$a.hasClass('action')) return false;

    if (!G.currentDeviceInfo) return false;

    var presetId = parseInt($('#preset-list').val());

    if ($a.hasClass('del')) { // remove preset
      if (!presetId) return false;
      $('#player').addClass('hide');
      var del = confirm($.i18n.prop('del-preset'));
      $('#player').removeClass('hide');

      if (!del) return false;
      var flag = J.delPreset(G.currentDeviceInfo.channel, presetId);
      if (flag) updatePresetList();
    }

    if ($a.hasClass('run')) { // move to preset
      if (!presetId) return false;
      J.applyPreset(G.currentDeviceInfo.channel, presetId);
    }

    var options = [];
    for (var i = 1; i <= 255; i++) {
      options.push('<option value="' + i + '">' + i + '</option>');
    }

    var html = ''
    + '<div>'
    + '<label for="preset-id">' + $.i18n.prop('id') + '</label>'
    + '<select id="preset-id">' + options.join('') + '</select>'
    + '</div>'
    + '<div>'
    + '<label for="preset-name">' + $.i18n.prop('name') + '</label>'
    + '<input type="input" id="preset-name">'
    + '</div>'
    + '';

    if ($a.hasClass('add')) { // add preset
      J.popWin($.i18n.prop('add-preset'), html, function ($win) {
        $win.find('.ok').click(function () {
          var id = parseInt($win.find('#preset-id').val());
          var name = $win.find('#preset-name').val();
          if (id) {
            if (name) name = $.trim(name);
            var status = J.addPreset(G.currentDeviceInfo.channel, id, name);
            if (status === true) {
              $('#player').addClass('hide');
              alert($.i18n.prop('add-preset-success'));
              $('#player').removeClass('hide');
              updatePresetList();
            } else {
              alert($.i18n.prop($.trim(status).replace(/ /g, '-')));
            }
          }
        });
      }, {
        className: 'add-preset'
      });
    }
  });

  $(window).bind("beforeunload", function(event) {
    if (X) {
      X.JSDisconnectAll();
    }
  });

  $('#ptz-panel .patrol').click(function (event) {
    var $a = $(event.target).parent();
    if (!$a.hasClass('action')) return false;

    if (!G.currentDeviceInfo) return false;

    var patrolId = parseInt($('#patrol-list').val());

    if ($a.hasClass('start')) {
      if (patrolId >= 0)
        J.startPatrol(G.currentDeviceInfo.channel, patrolId);
    }

    if ($a.hasClass('stop')) {
      if (patrolId >= 0)
        J.stopPatrol(G.currentDeviceInfo.channel, patrolId);
    }

    if ($a.hasClass('edit')) {
      var presetOptions = $('#preset-list').html();
      var addRow = ''
      + '  <tr>'
      + '    <td>'
      + '      <select>'
      + presetOptions
      + '      </select>'
      + '    </td>'
      + '    <td><input type="text" value="50" /></td>'
      + '    <td>'
      + '      <a href="#" title="' + $.i18n.prop('add') + '"><img class="action add" src="/img/icon-inc.png" /></a>'
      + '    </td>'
      + '  </tr>';

      var titleRow = ''
      + '  <tr class="add-row">'
      + '    <th>' + $.i18n.prop('preset') + '</th>'
      + '    <th>' + $.i18n.prop('stay-time') + '</th>'
      + '    <th>' + $.i18n.prop('action') + '</th>'
      + '  </tr>';

      var presetHtml = [];
      $.each(J.getPatrolPresets(G.currentDeviceInfo.channel, patrolId), function (i, preset) {
        presetHtml.push(''
                        + '  <tr class="preset">'
                        + '    <td>' + preset.name + '</td>'
                        + '    <td>' + preset.staytime + '</td>'
                        + '    <td>'
                        + '      <a href="#" title="' + $.i18n.prop('remove') + '"><img class="action del" src="/img/icon-stop.png" /></a>'
                        + '    </td>'
                        + '  </tr>');
      });
      var presets = presetHtml.join('');

      var html = ''
      + '<table>'
      + titleRow
      + presets
      + addRow
      + '</table>';

      J.popWin($.i18n.prop('add-patrol-preset'), html, function ($win) {
        $win.find('table').click(function (event) {
          var $target = $(event.target);
          if (!$target.hasClass('action')) return false;

          var channel = G.currentDeviceInfo.channel;
          var patrolId = $('#patrol-list').val();

          if ($target.hasClass('add')) {
            var tr = $target.parent().parent().parent();

            var presetId = parseInt(tr.find('option:selected').val());

            var $timeInput = tr.find('input');
            var time = parseInt($timeInput.val()) || 50;
            $timeInput.val(time);

            if (time > 65534) time = 65534;
            var presetRow = ''
            + '  <tr class="preset">'
            + '    <td>' + tr.find('option:selected').text() + '</td>'
            + '    <td>' + time + '</td>'
            + '    <td>'
            + '      <a href="#" title="del"><img class="action del" src="/img/icon-stop.png" /></a>'
            + '    </td>'
            + '  </tr>';
            var status = J.addPatrolPreset(channel, patrolId, presetId, time);
            if (status === true) {
              $(presetRow).insertBefore(tr);
              $target[0].scrollIntoView(true);
            } else {
              alert($.i18n.prop($.trim(status).replace(/ /g, '-')));
            }
          }

          if ($target.hasClass('del')) {
            var tr = $target.parent().parent().parent();
            var presetIndex = tr.find('td').first().data('id');
            var presets = tr.parent().find('.preset');
            if (J.delPatrolPreset(channel, patrolId, presets.index(tr))) {
              tr.remove();
            }
          }
        });
      }, {
        className: 'add-patrol-preset'
      });
    }
  });

  $.ajax({
    type: 'HEAD',
    async: true,
    url: '/img/logo.png',
    statusCode: {
      200: function () {
        $('#header .logo').html('<img src="/img/logo.png" />');
      }
    }
  });
});
