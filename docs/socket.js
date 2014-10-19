$(function() {

  var socket_di;

  function get_appropriate_ws_url()
  {
    var pcol;
    var u = document.URL;
    console.log("Getting URL.");


     //We open the websocket encrypted if this page came on an
     //https:// url itself, otherwise unencrypted


    if (u.substring(0, 5) == "https") {
      pcol = "wss://";
      u = u.substr(8);
    } else {
      pcol = "ws://";
      if (u.substring(0, 4) == "http")
        u = u.substr(7);
    }

    u = u.split('/');
    console.log("URL is "+ pcol + u[0]);

    return pcol + u[0] + ":7681";
  }

  function reset() {
    socket_di.send("reset\n");
  }

  function MM_preloadImages() { //v3.0
    var d=document; if(d.images){ if(!d.MM_p) d.MM_p=new Array();
      var i,j=d.MM_p.length,a=MM_preloadImages.arguments; for(i=0; i<a.length; i++)
      if (a[i].indexOf("#")!=0){ d.MM_p[j]=new Image; d.MM_p[j++].src=a[i];}}
  }

  function MM_swapImgRestore() { //v3.0
    var i,x,a=document.MM_sr; for(i=0;a&&i<a.length&&(x=a[i])&&x.oSrc;i++) x.src=x.oSrc;
  }

  function MM_findObj(n, d) { //v4.01
    var p,i,x;  if(!d) d=document; if((p=n.indexOf("?"))>0&&parent.frames.length) {
      d=parent.frames[n.substring(p+1)].document; n=n.substring(0,p);}
    if(!(x=d[n])&&d.all) x=d.all[n]; for (i=0;!x&&i<d.forms.length;i++) x=d.forms[i][n];
    for(i=0;!x&&d.layers&&i<d.layers.length;i++) x=MM_findObj(n,d.layers[i].document);
    if(!x && d.getElementById) x=d.getElementById(n); return x;
  }

  function MM_swapImage() { //v3.0
    var i,j=0,x,a=MM_swapImage.arguments; document.MM_sr=new Array; for(i=0;i<(a.length-2);i+=3)
     if ((x=MM_findObj(a[i]))!=null){document.MM_sr[j++]=x; if(!x.oSrc) x.oSrc=x.src; x.src=a[i+2];}
  }

  function MM_popupMsg(msg) { //v1.0
    console.log(msg);
  }

  function close_curtain(num) {         
    data = "ClsCur" + num;
    socket_di.send(data);
    console.log("send "+data);
  }

  function open_curtain(num) {
    data = "OpnCur" + num;
    socket_di.send(data);
    console.log("send "+data);
  }

  function open_light(num) {
    data = "OpnLight" + num;
    socket_di.send(data);
    console.log("send "+data);
  }

  function close_light(num) {
    data = "ClsLight" + num;
    socket_di.send(data);
    console.log("send "+data);
  }

  console.log("socket.js return.");
	console.log("inital all the inputs.");
  // initialize all the inputs
  $('input[type="checkbox"],[type="radio"]').not('#create-switch').bootstrapSwitch();

  // dimension
  $('#btn-size-regular-switch').on('click', function () {
    $('#dimension-switch').bootstrapSwitch('setSizeClass', '');
  });
  $('#btn-size-mini-switch').on('click', function () {
    $('#dimension-switch').bootstrapSwitch('setSizeClass', 'switch-mini');
  });
  $('#btn-size-small-switch').on('click', function () {
    $('#dimension-switch').bootstrapSwitch('setSizeClass', 'switch-small');
  });
  $('#btn-size-large-switch').on('click', function () {
    $('#dimension-switch').bootstrapSwitch('setSizeClass', 'switch-large');
  });

  // state
  $('#toggle-state-switch-button').on('click', function () {
    $('#toggle-state-switch').bootstrapSwitch('toggleState');
  });
  $('#toggle-state-switch-button-on').on('click', function () {
    $('#toggle-state-switch').bootstrapSwitch('setState', true);
  });
  $('#toggle-state-switch-button-off').on('click', function () {
    $('#toggle-state-switch').bootstrapSwitch('setState', false);
  });
  $('#toggle-state-switch-button-state').on('click', function () {
    alert($('#toggle-state-switch').bootstrapSwitch('state'));
  });

  // destroy
  $('#btn-destroy-switch').on('click', function () {
    $('#destroy-switch').bootstrapSwitch('destroy');
    $(this).remove();
  });
  // CREATE
  $('#btn-create').on('click', function () {
    $('#create-switch').bootstrapSwitch();
    $(this).remove();
  });

  // activation
  var $disable = $('#disable-switch');
  $('#btn-disable-is').on('click', function () {
    alert($disable.bootstrapSwitch('isDisabled'));
  });
  $('#btn-disable-toggle').on('click', function () {
    $disable.bootstrapSwitch('toggleDisabled');
  });
  $('#btn-disable-set').on('click', function () {
    $disable.bootstrapSwitch('setDisabled', true);
  });
  $('#btn-disable-remove').on('click', function () {
    $disable.bootstrapSwitch('setDisabled', false);
  });

  // readonly
  var $readonly = $('#readonly-switch');
  $('#btn-readonly-is').on('click', function () {
    alert($readonly.bootstrapSwitch('isReadOnly'));
  });
  $('#btn-readonly-toggle').on('click', function () {
    $readonly.bootstrapSwitch('toggleReadOnly');
  });
  $('#btn-readonly-set').on('click', function () {
    $readonly.bootstrapSwitch('setReadOnly', true);
  });
  $('#btn-readonly-remove').on('click', function () {
    $readonly.bootstrapSwitch('setReadOnly', false);
  });

  // label
  $('#btn-label-on-switch').on('click', function() {
    $('#label-switch').bootstrapSwitch('setOnLabel', 'I');
  });
  $('#btn-label-off-switch').on('click', function() {
    $('#label-switch').bootstrapSwitch('setOffLabel', 'O');
  });

  $('#label-toggle-switch').on('click', function(e, data) {
    $('.label-toggle-switch').bootstrapSwitch('toggleState');
  });
  $('#curtain_1').on('switch-change', function(e, data) {
    console.log(data.value);
    if(data.value == true) {
    	open_curtain(1);
    }
    else {
    	close_curtain(1);
    }
  });

    $('#curtain_2').on('switch-change', function(e, data) {
    console.log(data.value);
    if(data.value == true) {
      open_curtain(2);
    }
    else {
      close_curtain(2);
    }
  });

  //   $('#curtain_3').on('switch-change', function(e, data) {
  //   console.log(data.value);
  //   if(data.value == true) {
  //     open_curtain(3);
  //   }
  //   else {
  //     close_curtain(3);
  //   }
  // });

  //   $('#curtain_4').on('switch-change', function(e, data) {
  //   console.log(data.value);
  //   if(data.value == true) {
  //     open_curtain(4);
  //   }
  //   else {
  //     close_curtain(4);
  //   }
  // });

    $('#light_1').on('switch-change', function(e, data) {
    console.log(data.value);
    if(data.value == true) {
      open_light(1);
    }
    else {
      close_light(1);
    }
  });

    $('#light_2').on('switch-change', function(e, data) {
    console.log(data.value);
    if(data.value == true) {
      open_light(2);
    }
    else {
      close_light(2);
    }
  });

    $('#light_3').on('switch-change', function(e, data) {
    console.log(data.value);
    if(data.value == true) {
      open_light(3);
    }
    else {
      close_light(3);
    }
  });

    $('#light_4').on('switch-change', function(e, data) {
    console.log(data.value);
    if(data.value == true) {
      open_light(4);
    }
    else {
      close_light(4);
    }
  });

  // event handler
  $('#switch-change').on('switch-change', function (e, data) {
    var $element = $(data.el),
      value = data.value;

    console.log(e, $element, value);
  });

  // color
  $('#btn-color-on-switch').on('click', function() {
    $('#change-color-switch').bootstrapSwitch('setOnClass', 'success');
  });
  $('#btn-color-off-switch').on('click', function() {
    $('#change-color-switch').bootstrapSwitch('setOffClass', 'danger');
  });

  // animation
  $('#btn-animate-switch').on('click', function() {
    $('#animated-switch').bootstrapSwitch('setAnimated', true);
  });
  $('#btn-dont-animate-switch').on('click', function() {
    $('#animated-switch').bootstrapSwitch('setAnimated', false);
  });

  // radio
  $('.radio1').on('switch-change', function () {
    $('.radio1').bootstrapSwitch('toggleRadioState');
  });
  $('.radio2').on('switch-change', function () {
    $('.radio2').bootstrapSwitch('toggleRadioStateAllowUncheck', true);
  });



//dumb increment protocol
	

	console.log("running get ws_url");
	if (typeof MozWebSocket != "undefined") {
		socket_di = new MozWebSocket(get_appropriate_ws_url(),
				   "dumb-increment-protocol");
	} else {
		socket_di = new WebSocket(get_appropriate_ws_url(),
				   "dumb-increment-protocol");
	}

	console.log("Before socket_di onopen.");
	try {
		socket_di.onopen = function() {
			console.log("running onopen.");
			socket_di.send("Sync");
      window.open('www.techbrood.com','_blank');
      rptk_oneclick();
      console.log("rptk_oneclick");
      // $('#curtain_1').bootstrapSwitch('setState', false);
      // $('#curtain_2').bootstrapSwitch('setState', false);
      // $('#light_1').bootstrapSwitch('setState', false);
      // $('#light_2').bootstrapSwitch('setState', false);
      // $('#light_3').bootstrapSwitch('setState', false);
      // $('#light_4').bootstrapSwitch('setState', false);

		} 

		socket_di.onmessage =function got_packet(msg) {
    console.log("rcv data from server:" + msg.data);
      if(msg.data == "ClsCur1") {
        $('#curtain_1').bootstrapSwitch('setState', false);
        console.log("setState of curtain_1 to false");
      }
      else if(msg.data == "OpnCur1") {
        $('#curtain_1').bootstrapSwitch('setState', true);
        console.log("setState of curtain_1 to true");
      }
      else if(msg.data == "ClsCur2") {
        $('#curtain_2').bootstrapSwitch('setState', false);
        console.log("setState of curtain_2 to false");
      }
      else if(msg.data == "OpnCur2") {
        $('#curtain_2').bootstrapSwitch('setState', true);
        console.log("setState of curtain_2 to true");
      }
      else if(msg.data == "ClsLight1") {
        $('#light_1').bootstrapSwitch('setState', false);
        console.log("setState of light_1 to false");
      }
      else if(msg.data == "OpnLight1") {
        $('#light_1').bootstrapSwitch('setState', true);
        console.log("setState of light_1 to true");
      }
      else if(msg.data == "ClsLight2") {
        $('#light_2').bootstrapSwitch('setState', false);
        console.log("setState of light_2 to false");
      }
      else if(msg.data == "OpnLight2") {
        $('#light_2').bootstrapSwitch('setState', true);
        console.log("setState of light_2 to true");
      }
      else if(msg.data == "ClsLight3") {
        $('#light_3').bootstrapSwitch('setState', false);
        console.log("setState of light_3 to false");
      }
      else if(msg.data == "OpnLight3") {
        $('#light_3').bootstrapSwitch('setState', true);
        console.log("setState of light_3 to true");
      }
      else if(msg.data == "ClsLight4") {
        $('#light_4').bootstrapSwitch('setState', false);
        console.log("setState of light_4 to false");
      }
      else if(msg.data == "OpnLight4") {
        $('#light_4').bootstrapSwitch('setState', true);
        console.log("setState of light_4 to true");
      }
      else{
        console.log("rcv invalid data from server:" + msg.data);

      }
  
			// if(msg.data.substring(0, 3) == "AM0"){
			// 	document.getElementById("station").value = "AM:" + msg.data.substr(3) + "KHz" + "\n";
			// } else if(msg.data.substring(0, 3) == "FM1"){
			// 	document.getElementById("station").value = "FM1:" + msg.data.substr(3) + "MHz" + "\n";
			// } else if(msg.data.substring(0, 3) == "FM2"){
			// 	document.getElementById("station").value = "FM2:" + msg.data.substr(3) + "MHz" + "\n";
			// } else if(msg.data.substring(0, 3) == "Vol"){
			// 	document.getElementById("volume").value = "Vol:" + msg.data.substr(3) + "\n";
			// } else if(msg.data.substring(0, 6) == "Preset"){
			// 	if(msg.data.substring(6, 7) == "0"){
			// 		document.getElementById("preset0_frequency").value = msg.data.substr(7) + "\n";
			// 	} else if(msg.data.substring(6, 7) == "1"){
			// 		document.getElementById("preset1_frequency").value = msg.data.substr(7) + "\n";
			// 	} else if(msg.data.substring(6, 7) == "2"){
			// 		document.getElementById("preset2_frequency").value = msg.data.substr(7) + "\n";
			// 	} else if(msg.data.substring(6, 7) == "3"){
			// 		document.getElementById("preset3_frequency").value = msg.data.substr(7) + "\n";
			// 	} else{
			// 		//do nothing
			// 	}
			// } 
			//document.getElementById("volume").textContent = msg.data + "\n";
			
			//document.getElementById("station").textContent = msg.data + "\n";
		} 

		socket_di.onclose = function(){
		}
	} catch(exception) {
		alert('<p>Error' + exception);  
	}


});

