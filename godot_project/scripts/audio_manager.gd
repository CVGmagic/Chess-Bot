extends Node


@onready var button_clicked_sound: AudioStreamPlayer = $ButtonClickedSound


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass


func play_button_clicked() -> void:
	button_clicked_sound.play()
