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
});