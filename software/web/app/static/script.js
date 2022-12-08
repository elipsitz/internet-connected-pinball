$(document).ready(function() {
    $(".set-player-name").click(function() {
        let element = $(this);
        let player_name = window.prompt("Enter player initials:");
        if (player_name != null && player_name != "") {
            player_name = player_name.toUpperCase();
            $.post({
                url: "/api/v1/set_player_name",
                data: {
                    "score_id": $(this).data("score-id"),
                    "player_name": player_name,
                }
            })
            .done(function() {
                element.parent().empty().text(player_name);
            })
            .fail(function(_, _, xhr) {
                alert("Error: " + xhr);
            });
        }
    });

    $("th.sortable").click(function() {
        let key = $(this).data("key");
        let order = "desc";
        if ($(this).hasClass("desc")) {
            order = "asc";
        }
        let searchParams = new URLSearchParams(window.location.search)
        searchParams.set("sort", key);
        searchParams.set("order", order);
        window.location.search = searchParams.toString();
    });
});