$(document).ready(function() {
    $(".set-player-name").click(function() {
        const element = $(this);
        const score_id = element.data("score-id");
        const modal = new bootstrap.Modal("#player-name-modal");
        const input = $("#player-name-input");
        input.val("");
        $("#save-player-name").off("click");
        $("#save-player-name").click(function() {
            let player_name = input.val().toUpperCase();

            $.post({
                url: "/api/v1/set_player_name",
                data: {
                    "score_id": score_id,
                    "player_name": player_name,
                }
            })
            .done(function() {
                element.parent().empty().text(player_name);
            })
            .fail(function(_, _, xhr) {
                alert("Error: " + xhr);
            })
            .always(function() {
                modal.hide();
            })
        });
        modal.show();
    });
    $('#player-name-modal').on("shown.bs.modal", () => {
        $("#player-name-input").focus();
    });
});