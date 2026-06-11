extends Control

@onready var play_button: Button = $VBoxContainer/PlayButton
@onready var options_button: Button = $VBoxContainer/OptionsButton


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	play_button.pressed.connect(GameManager.start_game.bind())
	options_button.pressed.connect(GameManager.go_to_options.bind())


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
