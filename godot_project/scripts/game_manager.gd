extends Node


@onready var anim_player: AnimationPlayer = $AnimationPlayer
@onready var audio_manager: Node = $AudioManager

enum ColorChoice { WHITE, BLACK, RANDOM }
enum BotVersion { MINIMAX_EASY, ALPHABETA_MEDIUM, NEURAL_HARD }

# Global settings holding the player's choices
var selected_color: ColorChoice = ColorChoice.WHITE
var selected_bot: BotVersion = BotVersion.MINIMAX_EASY

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.


func start_game() -> void:
	if selected_color == ColorChoice.RANDOM:
		selected_color = ColorChoice.WHITE if randf() > 0.5 else ColorChoice.BLACK
	
	button_clicked_audio()
	
	anim_player.play("fade_to_black")
	await anim_player.animation_finished
	
	get_tree().change_scene_to_file("res://scenes/chess_game.tscn")
	
	anim_player.play("fade_from_black")


func go_to_options() -> void:
	button_clicked_audio()
	
	anim_player.play("fade_to_black")
	await anim_player.animation_finished
	
	get_tree().change_scene_to_file("res://scenes/options_menu.tscn")
	
	anim_player.play("fade_from_black")


func go_to_start() -> void:
	button_clicked_audio()
	
	anim_player.play("fade_to_black")
	await anim_player.animation_finished
	
	get_tree().change_scene_to_file("res://scenes/start_menu.tscn")
	
	anim_player.play("fade_from_black")


func button_clicked_audio() -> void:
	audio_manager.play_button_clicked()
