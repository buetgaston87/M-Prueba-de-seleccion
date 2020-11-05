$( document ).ready(function() {         
    $('#divDate').datepicker({
        format: 'yyyy/mm/dd'
    }); 

    $("[data-hide]").on("click", function(){
        $(this).closest("." + $(this).attr("data-hide")).hide();
    });
    
    var request = $.ajax({
        url: 'http://localhost:8888/api/cities',
        method: "GET",
        dataType: "json"
    });
    request.done(function( response ) {
        var cities = response.cities;
        cities.forEach(function(e, i){
            $('#selectCity').append($('<option></option>').val(e).text(e));
        });
    });
    request.fail(function( response ) {
        alert("Fallo la conexión");
    });
});

function searchData () {                
    $('#jsonResopnse').text(""); 
    var selectedCity = $('#selectCity');
    var selectedDate = $('#selectDate');
    var selectedUnit = $('#selectUnit');

    if (selectedCity.val() == null) {
        showAlert("Por favor completar campo Ciudad!");
        return;
    }
    if (selectedDate.val() == "") {
        showAlert("Por favor completar campo Fecha!");
        return;
    }
    if (selectedUnit.val() == null) {
        showAlert("Por favor completar campo Unidad!");
        return;
    }
    $("#div_alert").hide();
    
    var url = 'http://localhost:8888/api/search?date='+selectDate.value+'&city='+selectCity.value+'&unit='+selectUnit.value;
    var request = $.ajax({
        url: url,
        method: "GET",
        dataType: "json"
    });
    request.done(function( response ) {
        showJsonResponse(response);
    });
    request.fail(function( response ) {
        alert("Fallo la conexión");
    });

}
function showJsonResponse(response) {                
    var textedJson = JSON.stringify(response, undefined, 4);
    $('#jsonResopnse').text(textedJson);
    $('#divTextbox').css("visibility", "visible");                
}
function showAlert(message){
    $("#text").text(message);
    $("#div_alert").show();
}