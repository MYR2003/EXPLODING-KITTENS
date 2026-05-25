provider "aws" {
  region = "us-east-1" # Adjust to your preferred region
}

resource "aws_security_group" "game_sg" {
  name        = "exploding_kittens_sg"
  description = "Allow game traffic on 8080 and SSH"

  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    from_port   = 8080
    to_port     = 8080
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

resource "aws_instance" "game_server" {
  ami                    = "ami-05cf1e9f73fbad2e2" # Ubuntu 22.04 LTS (us-east-1)
  instance_type          = "t3.micro"
  vpc_security_group_ids = [aws_security_group.game_sg.id]
  key_name               = "PPYD_keys"     # Replace with your actual key pair name

  tags = {
    Name = "ExplodingKittensServer"
  }
}

output "server_public_ip" {
  value       = aws_instance.game_server.public_ip
  description = "Share this IP with your players"
}