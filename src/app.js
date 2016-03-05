var myAPIKey = '';

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // URL
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude + '&appid=' + myAPIKey;

  // Отправляем запрос OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText содержит JSON обьект с информацией о погоде
      var json = JSON.parse(responseText);

      // Температура в Кельвинах(корректировка)
      var temperature = Math.round(json.main.temp - 273.15);
      console.log("Temperature is " + temperature);

      // Условия
      var conditions = json.weather[0].main;      
      console.log("Conditions are " + conditions);
      
      // Собираем словарь,используя ключи
      var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_CONDITIONS": conditions
      };

      // Отправить Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// При открытие циферблата
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
    
     // Получаем погоду
    getWeather();
  }
);

// При получении сообщения
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
  }                     
);
